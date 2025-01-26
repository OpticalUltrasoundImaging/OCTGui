#pragma once

#include "Common.hpp"
#include "DAQ.hpp"
#include "OCTData.hpp"
#include "RingBuffer.hpp"
#include <QGridLayout>
#include <QLabeL>
#include <QObject>
#include <QPushButton>
#include <QSpinBox>
#include <QThread>
#include <QVBoxLayout>
#include <QWidget>
#include <memory>

#ifdef OCTGUI_HAS_ALAZAR

namespace OCT {

struct AcquisitionParams {
  int maxFrames = 400;
};

class AcquisitionControllerObj : public QObject {
  Q_OBJECT
public:
  explicit AcquisitionControllerObj(
      const std::shared_ptr<RingBuffer<OCTData<Float>>> &buffer);

  auto &daq() { return m_daq; }
  bool isAcquiring() const { return acquiring; }
  void startAcquisition(AcquisitionParams params);

  void stopAcquisition() { acquiring = false; }

Q_SIGNALS:
  void sigAcquisitionStarted();
  void sigAcquisitionFinished();

private:
  daq::DAQ m_daq;
  std::atomic<bool> acquiring{false};
};

class AcquisitionController : public QWidget {
  Q_OBJECT
public:
  explicit AcquisitionController(
      const std::shared_ptr<RingBuffer<OCTData<Float>>> &buffer);

  AcquisitionController(const AcquisitionController &) = delete;
  AcquisitionController(AcquisitionController &&) = delete;
  AcquisitionController &operator=(const AcquisitionController &) = delete;
  AcquisitionController &operator=(AcquisitionController &&) = delete;

  ~AcquisitionController() override;

private:
  AcquisitionControllerObj m_controller;
  QThread m_controllerThread;

  // UI
  QPushButton *m_btnStartStopAcquisition;
  QPushButton *m_btnSaveOrDisplay;

  // Acquisition params
  AcquisitionParams m_acqParams;
  QSpinBox *m_sbMaxFrames;
};
} // namespace OCT

#endif