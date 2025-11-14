#pragma once

#include <cassert>
#include <cstring>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <optional>
#include <regex>
#include <span>
#include <stdexcept>
#include <string>

namespace OCT {

namespace fs = std::filesystem;

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

[[nodiscard]] inline std::string getSequenceName(const fs::path &path) {
  return (path.parent_path().stem() / path.stem()).string();
}

namespace detail {

inline std::optional<std::string> readFile(const fs::path &path,
                                           std::streamsize offset,
                                           std::streamsize bytesToRead,
                                           void *dst) {
  std::ifstream fs(path, std::ios::binary);
  fs.seekg(offset);
  // NOLINTNEXTLINE(*-reinterpret-cast)
  fs.read(reinterpret_cast<char *>(dst), bytesToRead);

  if (fs.eof()) {
    return fmt::format("EOF reached while reading {}. Read {}/{} bytes. "
                       "Attempted to seek to {}",
                       path.string(), fs.gcount(), bytesToRead, offset);
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

} // namespace detail

/**
Read a sequence of old .dat files

One imaging sequence is grouped into normally 20 files.
Each file normally has 20 frames and each frame consists of Ascans with 6144
(2048*3) samples each. The in vivo probe acquires 2200 Ascans per frame. The
ex vivo probe acquires 2500 Ascans per frame.
 */
struct DatFileReader {
  using T = uint16_t;
  static constexpr size_t ALineSize = 2048LL * 3;

  DatFileReader() = default;
  explicit DatFileReader(const std::span<const fs::path> files)
      : m_files(files.begin(), files.end()) {
    if (!files.empty()) {
      determineFrameSize();
    } else {
      std::cerr << "DatFileReader received empty span of fs::path\n";
    }
  }

  static DatFileReader readDatDirectory(const fs::path &directory) {
    DatFileReader reader;
    try {
      if (fs::exists(directory) && fs::is_directory(directory)) {
        reader.m_seq = getSequenceName(directory);

        for (const auto &entry : fs::directory_iterator(directory)) {
          if (entry.is_regular_file() && entry.path().extension() == ".dat") {
            reader.m_files.push_back(entry.path());
          }
        }
      }

      if (!reader.m_files.empty()) {
        reader.determineFrameSize();
      }

    } catch (const fs::filesystem_error &e) {
      std::cerr << "Filesystem error: " << e.what() << '\n';
    } catch (const std::exception &e) {
      std::cerr << "Exception: " << e.what() << '\n';
    }

    return reader;
  }

  static DatFileReader readBinFile(const fs::path &filepath) {
    DatFileReader reader;
    if (fs::exists(filepath)) {
      // Sequence
      reader.m_files = {filepath};
      reader.m_seq = getSequenceName(filepath);

      // Parse file name
      {
        const auto stem = filepath.stem().string();
        std::regex re(R"rgx(OCT\d+_(\d+))rgx");
        std::smatch match;
        int linesPerFrame = 0;
        if (std::regex_search(stem, match, re)) {
          linesPerFrame = std::stoi(match[1].str());
        }
        reader.determineFrameSize(linesPerFrame);
      }
    }

    return reader;
  }

  [[nodiscard]] bool ok() const {
    return !m_files.empty() && m_framesPerFile != 0 && m_linesPerFrame != 0;
  }

  // Get the number of frames available.
  [[nodiscard]] size_t size() const { return m_files.size() * m_framesPerFile; }

  [[nodiscard]] size_t samplesPerFrame() const {
    return m_linesPerFrame * ALineSize;
  }

  // Get the size of one frame in bytes
  [[nodiscard]] size_t frameSizeBytes() const {
    return samplesPerFrame() * sizeof(T);
  }

  [[nodiscard]] auto seq() const -> const std::string & { return m_seq; }

  // Read `frameIdx` into buffer `dst`
  [[nodiscard]] inline std::optional<std::string>
  read(size_t frameStartIdx, size_t numFrames, std::span<T> dst) const {
    if (numFrames < 1) {
      return "Must read at least 1 frame.";
    }
    if (dst.size() < (samplesPerFrame() * numFrames)) {
      return "Dst buffer too small!";
    }
    if (frameStartIdx + numFrames > size()) {
      return "Trying to read past the end of file.";
    }

    const auto &path = m_files[frameStartIdx / m_framesPerFile];
    // NOLINTBEGIN(*-narrowing-conversions)
    const std::streamsize offset =
        (frameStartIdx % m_framesPerFile) * frameSizeBytes();
    const std::streamsize bytesToRead = frameSizeBytes() * numFrames;
    // NOLINTEND(*-narrowing-conversions)
    return detail::readFile(path, offset, bytesToRead, dst.data());
  }

private:
  std::vector<fs::path> m_files;
  std::string m_seq{"empty"};
  size_t m_framesPerFile{};
  size_t m_linesPerFrame{};

  // Checks the size of the first file to set `framesPerFile` and
  // `linesPerFile`
  void determineFrameSize(int linesPerFrame = 0) {
    if (!m_files.empty()) {
      const auto samples = fs::file_size(m_files[0]) / sizeof(T);

      // Invivo probe acquires 2200 Ascans per Bscan
      // Exvivo probe acquires 2500 Ascans per Bscan
      if ((samples % ALineSize) == 0) {
        const auto totalLines = samples / ALineSize;

        if (linesPerFrame == 0) {

          // NOLINTBEGIN(*-magic-numbers)
          if ((samples % 2500) == 0) {
            // Ex vivo probe data
            m_linesPerFrame = 2500;
          } else if ((samples % 2200) == 0) {
            // In vivo probe data
            m_linesPerFrame = 2200;
          } else {
            throw std::invalid_argument("Unknown linesPerFrame.");
          }
          // NOLINTEND(*-magic-numbers)

        } else {
          m_linesPerFrame = linesPerFrame;
        }

        m_framesPerFile = totalLines / m_linesPerFrame;
      } else {
        std::cerr << "Invalid file size: " << samples
                  << ", not divisible by A line size " << ALineSize << ".\n";
      }
    }
  }
};

} // namespace OCT
