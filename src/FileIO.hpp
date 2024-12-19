#pragma once

#include <cassert>
#include <filesystem>
#include <fstream>
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

  std::vector<fs::path> files;
  size_t framesPerFile;
  size_t samplesPerFrame;

  // Get the number of frames available.
  [[nodiscard]] inline auto size() const {
    return files.size() * framesPerFile;
  }

  // Get the size of one frame in bytes
  [[nodiscard]] constexpr auto frameSizeBytes() const {
    return samplesPerFrame * sizeof(T);
  }

  // Read `frameIdx` into buffer `dst`
  inline void read(size_t frameStartIdx, size_t numFrames,
                   std::span<T> dst) const {
    assert(frameStartIdx >= 0 && frameStartIdx < framesPerFile * files.size());
    assert(dst.size() >= samplesPerFrame);

    const auto fileIdx = frameStartIdx / framesPerFile;
    frameStartIdx = frameStartIdx % framesPerFile;
    const std::streamsize offset = frameStartIdx * frameSizeBytes();

    std::ifstream fs(files[fileIdx]);
    fs.seekg(offset);
    fs.read(reinterpret_cast<char *>(dst.data()), frameSizeBytes());
  }
};

} // namespace OCT