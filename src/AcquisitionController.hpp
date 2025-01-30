#pragma once

#include "Common.hpp"
#include "DAQ.hpp"
#include "MotorDriver.hpp"
#include "OCTData.hpp"
#include "RingBuffer.hpp"
#include <QGridLayout>
#include <QGroupBox>
#include <QLabeL>
#include <QMap>
#include <QObject>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QString>
#include <QThread>
#include <QVBoxLayout>
#include <QWidget>
#include <memory>

#ifdef OCTGUI_HAS_ALAZAR

namespace OCT {

// NOLINTBEGIN(*-magic-numbers)
struct AcquisitionParams {
  int maxFrames = 400;
};
// NOLINTEND(*-magic-numbers)

class AcquisitionControllerObj : public QObject {
  Q_OBJECT

public:
  /*
  Support 3 acquisition modes:

    1. Free: just run the DAQ (no automatic motors). Can manually control
    motors
    2. 2D: Automatically start/stop rotary motor
    3. 3D: Automatically start/stop rotary and 3D motors
  */
  enum AcquisitionMode : std::uint8_t { ModeManual = 0, Mode2D, Mode3D };
  Q_ENUM(AcquisitionMode);

  explicit AcquisitionControllerObj(
      const std::shared_ptr<RingBuffer<OCTData<Float>>> &buffer,
      MotorDriver *motorDriver);

  auto &daq() { return m_daq; }
  bool isAcquiring() const { return m_acquiring; }
  void startAcquisition(AcquisitionParams params);

  void stopAcquisition() {
    m_acquiring = false;
    m_daq.setShouldStopAcquiring();
  }

Q_SIGNALS:
  void sigAcquisitionStarted();
  void sigAcquisitionFinished(QString filepath);

  void error(QString msg);

private:
  std::atomic<bool> m_acquiring{false};

  daq::DAQ m_daq;
  MotorDriver *m_motorDriver;
};

class AcquisitionController : public QWidget {
  Q_OBJECT
public:
  explicit AcquisitionController(
      const std::shared_ptr<RingBuffer<OCTData<Float>>> &buffer,
      MotorDriver *motorDriver);

  AcquisitionController(const AcquisitionController &) = delete;
  AcquisitionController(AcquisitionController &&) = delete;
  AcquisitionController &operator=(const AcquisitionController &) = delete;
  AcquisitionController &operator=(AcquisitionController &&) = delete;

  ~AcquisitionController() override;

  const auto &controller() const { return m_controller; }

protected:
  void closeEvent(QCloseEvent *event) override;

private:
  // Acquisition controller obj run in separate thread
  AcquisitionControllerObj m_controller;
  QThread m_controllerThread;

  // UI
  QGroupBox *m_gbMode;
  QMap<int, QRadioButton *> m_modeRadioBtns;

  QPushButton *m_btnStartStopAcquisition;
  QPushButton *m_btnSaveOrDisplay;

  // Acquisition params
  AcquisitionParams m_acqParams;
  QSpinBox *m_sbMaxFrames;
};
} // namespace OCT

#endif