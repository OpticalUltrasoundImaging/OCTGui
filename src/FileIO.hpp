#pragma once

#include <cassert>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>

namespace OCT {

namespace fs = std::filesystem;

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

  std::vector<fs::path> files;
  size_t framesPerFile{};
  size_t linesPerFrame{};

  // Glob all .dat files in `directory` as input
  explicit DatReader(const fs::path &directory) {
    try {
      if (fs::exists(directory) && fs::is_directory(directory)) {
        for (const auto &entry : fs::directory_iterator(directory)) {
          if (entry.is_regular_file() && entry.path().extension() == ".dat") {
            files.push_back(entry.path());
          }
        }
      }

      determineFrameSize();
    } catch (const fs::filesystem_error &e) {
      std::cerr << "Filesystem error: " << e.what() << '\n';
    } catch (const std::exception &e) {
      std::cerr << "Exception: " << e.what() << '\n';
    }
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
  inline void read(size_t frameStartIdx, size_t numFrames,
                   std::span<T> dst) const {
    assert(frameStartIdx >= 0 && frameStartIdx < framesPerFile * files.size());
    assert(dst.size() >= samplesPerFrame());

    const auto fileIdx = frameStartIdx / framesPerFile;
    frameStartIdx = frameStartIdx % framesPerFile;
    const std::streamsize offset = frameStartIdx * frameSizeBytes();

    std::ifstream fs(files[fileIdx]);
    fs.seekg(offset);
    fs.read(reinterpret_cast<char *>(dst.data()), frameSizeBytes());
  }

private:
  // Checks the size of the first file to set `framesPerFile` and `linesPerFile`
  void determineFrameSize() {
    if (!files.empty()) {
      const auto fileSize = fs::file_size(files[0]);

      // Invivo probe acquires 2200 Ascans per Bscan
      // Exvivo probe acquires 2500 Ascans per Bscan
      if ((fileSize % ALineSize) == 0) {
        const auto totalLines = fileSize / ALineSize;
        if ((fileSize % 2500) == 0) {
          // Ex vivo probe data
          linesPerFrame = 2500;
          framesPerFile = totalLines / linesPerFrame;
        }
        if ((fileSize % 2200) == 0) {
          // In vivo probe data
          linesPerFrame = 2200;
          framesPerFile = totalLines / linesPerFrame;
        }
      } else {
        std::cerr << "Invalid file size: " << fileSize
                  << ", not divisible by A line size " << ALineSize << ".\n";
      }
    }
  }
};

} // namespace OCT