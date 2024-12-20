#pragma once

#include <cassert>
#include <exception>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <optional>
#include <span>
#include <string>

namespace OCT {

namespace fs = std::filesystem;

// Get the directory name if path (which must be a directory).
// Handles trailing slashes
[[nodiscard]] inline std::string getDirectoryName(const fs::path &path) {
  std::string name;
  if (fs::is_directory(path)) {
    if (path.has_filename()) {
      name = path.filename().string();
    } else if (path.has_parent_path() && path.parent_path().has_filename()) {
      name = path.parent_path().filename().string();
    }
  }
  return name;
}

/**
Read a sequence of old .dat files

One imaging sequence is grouped into normally 20 files.
Each file normally has 20 frames and each frame consists of Ascans with 6144
(2048*3) samples each. The in vivo probe acquires 2200 Ascans per frame. The ex
vivo probe acquires 2500 Ascans per frame.
 */
struct DatReader {
  using T = uint16_t;
  static constexpr size_t ALineSize = 2048 * 3;

  std::string seq{"unknown"};
  std::vector<fs::path> files;
  size_t framesPerFile{};
  size_t linesPerFrame{};

  // Glob all .dat files in `directory` as input
  explicit DatReader(const fs::path &directory) {
    try {
      if (fs::exists(directory) && fs::is_directory(directory)) {
        seq = getDirectoryName(directory);

        for (const auto &entry : fs::directory_iterator(directory)) {
          if (entry.is_regular_file() && entry.path().extension() == ".dat") {
            files.push_back(entry.path());
          }
        }
      }

      if (!files.empty()) {
        determineFrameSize();
      }

    } catch (const fs::filesystem_error &e) {
      std::cerr << "Filesystem error: " << e.what() << '\n';
    } catch (const std::exception &e) {
      std::cerr << "Exception: " << e.what() << '\n';
    }
  }

  [[nodiscard]] bool ok() const {
    return !files.empty() && framesPerFile != 0 && linesPerFrame != 0;
  }

  explicit DatReader(const std::span<const fs::path> files)
      : files(files.begin(), files.end()) {
    if (!files.empty()) {
      determineFrameSize();
    } else {
      std::cerr << "DatReader received empty span of fs::path\n";
    }
  }

  // Get the number of frames available.
  [[nodiscard]] inline auto size() const {
    return files.size() * framesPerFile;
  }

  [[nodiscard]] auto samplesPerFrame() const {
    return linesPerFrame * ALineSize;
  }

  // Get the size of one frame in bytes
  [[nodiscard]] auto frameSizeBytes() const {
    return samplesPerFrame() * sizeof(T);
  }

  // Read `frameIdx` into buffer `dst`
  [[nodiscard]] inline std::optional<std::string>
  read(size_t frameStartIdx, size_t numFrames, std::span<T> dst) const {
    assert(numFrames >= 1);
    assert((frameStartIdx + numFrames) <= framesPerFile * files.size());
    assert(dst.size() >= samplesPerFrame());

    const auto fileIdx = frameStartIdx / framesPerFile;
    frameStartIdx = frameStartIdx % framesPerFile;
    const std::streamsize offset = frameStartIdx * frameSizeBytes();

    const auto &path = files[fileIdx];
    std::ifstream fs(path, std::ios::binary);
    fs.seekg(offset);
    fs.read(reinterpret_cast<char *>(dst.data()), frameSizeBytes());

    if (fs.eof()) {
      return fmt::format("EOF reached while reading {}. Read {}/{} bytes. "
                         "Attempted to seek to {}",
                         path.string(), fs.gcount(), frameSizeBytes(), offset);
    }
    if (fs.fail()) {
      return fmt::format("Error: Read failed (failbit set) while reading {}",
                         path.string());
    }
    if (fs.bad()) {
      return fmt::format(
          "Error: Critical I/O error (batbit set) while reading {}",
          path.string());
    }

    // Successful
    return std::nullopt;
  }

private:
  // Checks the size of the first file to set `framesPerFile` and `linesPerFile`
  void determineFrameSize() {
    if (!files.empty()) {
      const auto samples = fs::file_size(files[0]) / sizeof(T);

      // Invivo probe acquires 2200 Ascans per Bscan
      // Exvivo probe acquires 2500 Ascans per Bscan
      if ((samples % ALineSize) == 0) {
        const auto totalLines = samples / ALineSize;
        if ((samples % 2500) == 0) {
          // Ex vivo probe data
          linesPerFrame = 2500;
          framesPerFile = totalLines / linesPerFrame;
        }
        if ((samples % 2200) == 0) {
          // In vivo probe data
          linesPerFrame = 2200;
          framesPerFile = totalLines / linesPerFrame;
        }
      } else {
        std::cerr << "Invalid file size: " << samples
                  << ", not divisible by A line size " << ALineSize << ".\n";
      }
    }
  }
};

} // namespace OCT