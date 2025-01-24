/*
Implements a data acquisition interface
*/
#pragma once

#ifdef OCTGUI_HAS_ALAZAR

#include "Common.hpp"
#include "OCTData.hpp"
#include "RingBuffer.hpp"
#include <AlazarApi.h>
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
      : m_buffer(std::move(buffer)) {}

  DAQ(const DAQ &) = delete;
  DAQ(DAQ &&) = delete;
  DAQ &operator=(const DAQ &) = delete;
  DAQ &operator=(DAQ &&) = delete;

  ~DAQ();

  // Initialize the DAQ board (set clock, trigger, channels etc.)
  bool initHardware() noexcept;
  bool isInitialized() const noexcept { return board != nullptr; }

  // Must be called before acquisition
  bool prepareAcquisition() noexcept;
  bool acquire(int buffersToAcquire) noexcept;

  // CLean up resources allocated by prepareAcquisition
  void finishAcquisition() noexcept {
    if (m_fs.is_open()) {
      m_fs.close();
    }
  }

  void setSaveData(bool save) noexcept { m_saveData = save; }
  void setSaveDir(fs::path savedir) noexcept { m_savedir = std::move(savedir); }
  const fs::path &binpath() const noexcept { return m_lastBinfile; }
  const std::string &errMsg() const noexcept { return m_errMsg; }

private:
  // Ring buffer
  std::shared_ptr<RingBuffer<OCTData<Float>>> m_buffer;

  // Control states
  std::atomic<bool> shouldStopAcquiring{false};
  std::atomic<bool> acquiringData{false};

  // Alazar board handle
  void *board{};

  // Alazar buffers
  std::array<std::span<uint16_t>, 20> buffers{};

  // Channels to capture
  const U32 channelMask = CHANNEL_A;
  const int channelCount = 1;

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