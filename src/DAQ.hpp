/*
Implements a data acquisition interface
*/
#pragma once

#ifdef OCTGUI_HAS_ALAZAR

#include "Common.hpp"
#include "OCTData.hpp"
#include "RingBuffer.hpp"
#include <array>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <memory>
#include <span>
#include <string>

namespace OCT::daq {

namespace fs = std::filesystem;

std::string getDAQInfo();

class DAQ {
public:
  explicit DAQ(std::shared_ptr<RingBuffer<OCTData<Float>>> buffer)
      : m_ringBuffer(std::move(buffer)) {}

  DAQ(const DAQ &) = delete;
  DAQ(DAQ &&) = delete;
  DAQ &operator=(const DAQ &) = delete;
  DAQ &operator=(DAQ &&) = delete;

  ~DAQ();

  // Initialize the DAQ board (set clock, trigger, channels etc.)
  bool initHardware() noexcept;
  bool isInitialized() const noexcept { return board != nullptr; }

  // Must be called before acquisition
  bool prepareAcquisition(int maxBuffersToAcquire) noexcept;
  bool acquire(int buffersToAcquire,
               const std::function<void()> &callback) noexcept;

  // CLean up resources allocated by prepareAcquisition
  void finishAcquisition() noexcept {
    if (m_fs.is_open()) {
      m_fs.close();
    }
  }

  void setShouldStopAcquiring() { shouldStopAcquiring = true; }

  void setSaveData(bool save) noexcept { m_saveData = save; }
  bool isSavingData() const noexcept { return m_saveData; }
  void setSaveDir(fs::path savedir) noexcept { m_savedir = std::move(savedir); }
  const fs::path &binpath() const noexcept { return m_lastBinfile; }
  const std::string &errMsg() const noexcept { return m_errMsg; }

  // Get and set A line size
  uint32_t getRecordsPerBuffer() const { return recordsPerBuffer; }
  void setRecordsPerBuffer(uint32_t val) { recordsPerBuffer = val; }

private:
  // Ring buffer
  std::shared_ptr<RingBuffer<OCTData<Float>>> m_ringBuffer;

  // Control states
  std::atomic<bool> shouldStopAcquiring{false};
  std::atomic<bool> acquiringData{false};

  // Alazar board handle
  void *board{};

  // Alazar buffers
  static constexpr size_t num_buffers{16};
  std::array<std::span<uint16_t>, num_buffers> buffers{};

  uint32_t recordSize = 3 * 2048;   // ALine size
  uint32_t recordsPerBuffer = 2200; // ALines per BScan
  uint32_t channelMask{};

  // Sampling rate
  double samplesPerSec = 0.0;

  // Save to file
  bool m_saveData{true};
  std::fstream m_fs;
  fs::path m_savedir{"C:/Data/"};
  fs::path m_lastBinfile;

  // Last error message
  std::string m_errMsg;
};

} // namespace OCT::daq

#endif