#include "MotorDriver.hpp"
#include <qobjectdefs.h>
#ifdef OCTGUI_HAS_ALAZAR

#include "AcquisitionController.hpp"
#include "strOps.hpp"
#include <QButtonGroup>
#include <QComboBox>
#include <QMessageBox>
#include <QMetaEnum>
#include <QRadioButton>
#include <QThread>

namespace OCT {

AcquisitionControllerObj::AcquisitionControllerObj(
    const std::shared_ptr<RingBuffer<OCTData<Float>>> &buffer,
    MotorDriver *motorDriver)
    : m_daq(buffer), m_motorDriver(motorDriver) {}

void AcquisitionControllerObj::startAcquisition(AcquisitionParams params,
                                                AcquisitionMode mode) {
  m_acquiring = true;
  bool daqSuccess = true;

  daqSuccess = m_daq.initHardware();
  if (daqSuccess) {
    daqSuccess = m_daq.prepareAcquisition(params.maxFrames);
  }

  if (daqSuccess) {

    // Auto motor control
    if (mode == Mode2D || mode == Mode3D) {
      // m_motorDriver->setEnabled(false);
      QMetaObject::invokeMethod(m_motorDriver, &MotorDriver::setEnabled, false);

      // Start rotary motor
      // m_motorDriver->rotaryEnable(true);
      QMetaObject::invokeMethod(m_motorDriver, &MotorDriver::rotaryEnable,
                                true);

      // Wait 500ms for motor rotation to ramp up
      QThread::msleep(500);

      // if 3D, turn on 3D motor pulling
      if (mode == Mode3D) {
        QMetaObject::invokeMethod(m_motorDriver,
                                  &MotorDriver::handleDirectionButton, false);
        QMetaObject::invokeMethod(m_motorDriver,
                                  &MotorDriver::handleRunStopButton, true);
        // m_motorDriver->handleDirectionButton(false);
        // m_motorDriver->handleRunStopButton(true);
      }
    }

    Q_EMIT sigAcquisitionStarted();

    while (m_acquiring) {
      daqSuccess = m_daq.acquire(params.maxFrames, {});
      if (!daqSuccess) {
        const auto &daqErr = "DAQ error: " + m_daq.errMsg();
        if (!daqErr.empty()) {
          Q_EMIT error(QString::fromStdString(daqErr));
        }
        break;
      }

      if (m_daq.isSavingData()) {
        m_acquiring = false;
      }
    }

    // Clean up auto motor control
    if (mode == Mode2D || mode == Mode3D) {
      // m_motorDriver->rotaryEnable(false);
      QMetaObject::invokeMethod(m_motorDriver, &MotorDriver::rotaryEnable,
                                false);

      if (mode == Mode3D) {
        // m_motorDriver->handleRunStopButton(false);
        QMetaObject::invokeMethod(m_motorDriver,
                                  &MotorDriver::handleRunStopButton, false);
      }

      QMetaObject::invokeMethod(m_motorDriver, &MotorDriver::setEnabled, true);
    }
  }

  m_acquiring = false;
  m_daq.finishAcquisition();
  Q_EMIT sigAcquisitionFinished(toQString(m_daq.binpath()));
}

AcquisitionController::AcquisitionController(
    const std::shared_ptr<RingBuffer<OCTData<Float>>> &buffer,
    MotorDriver *motorDriver)
    : m_controller(buffer, motorDriver),

      m_gbMode(new QGroupBox("Acquisition mode")),

      m_btnAcquireBackgound(new QPushButton("Acquire background")),
      m_btnStartStopAcquisition(new QPushButton("Start")),
      m_btnSaveOrDisplay(new QPushButton("Saving")), m_sbMaxFrames(new QSpinBox)

{

  m_controller.moveToThread(&m_controllerThread);
  m_controllerThread.start(QThread::HighestPriority);

  auto *layout = new QHBoxLayout;
  setLayout(layout);

  // Mode selection
  {
    auto *radioLayout = new QVBoxLayout;
    m_gbMode->setLayout(radioLayout);
    layout->addWidget(m_gbMode);

    m_modeBtnGroup = new QButtonGroup(this);

    const auto metaEnum =
        QMetaEnum::fromType<AcquisitionControllerObj::AcquisitionMode>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
      const auto val = metaEnum.value(i);
      QString text = metaEnum.valueToKey(val);
      // Trim the "Mode" prefix from Mode enum names. E.g. "ModeFree" -> "Free"
      auto *rbtn = new QRadioButton(text.mid(4));
      radioLayout->addWidget(rbtn);
      m_modeBtnGroup->addButton(rbtn, val);
    }

    m_modeBtnGroup->button(AcquisitionControllerObj::ModeManual)
        ->setChecked(true);
  }

  auto *grid = new QGridLayout;
  layout->addLayout(grid);
  int row = 0;

  // Spinbox to set maxFrames
  {
    auto *lbl = new QLabel("Max frames");
    grid->addWidget(lbl, row, 0);
    grid->addWidget(m_sbMaxFrames, row, 1);

    // NOLINTBEGIN(*-numbers)
    m_sbMaxFrames->setMinimum(20);
    m_sbMaxFrames->setMaximum(2000);
    m_sbMaxFrames->setSingleStep(10);
    // NOLINTEND(*-numbers)
    m_sbMaxFrames->setValue(m_acqParams.maxFrames);

    connect(m_sbMaxFrames, &QSpinBox::valueChanged,
            [this](int val) { m_acqParams.maxFrames = val; });
  }

  // Acquire background
  row++;
  {
    grid->addWidget(m_btnAcquireBackgound, row, 0, 1, 2);
    connect(m_btnAcquireBackgound, &QPushButton::clicked, this, [this]() {
      // Can only be clicked when not currently acquiring
      this->setEnabled(false);

      m_acquiringBackground = true;

      // Only acquire 2 frames for background
      AcquisitionParams acqBackgroundParams;
      acqBackgroundParams.maxFrames = 2;

      QMetaObject::invokeMethod(
          &m_controller, &AcquisitionControllerObj::startAcquisition,
          acqBackgroundParams, AcquisitionControllerObj::ModeManual);
    });
  }

  // Start/stop button
  row++;
  {
    m_btnStartStopAcquisition->setStyleSheet("background-color: green");
    grid->addWidget(m_btnStartStopAcquisition, row, 0);

    connect(m_btnStartStopAcquisition, &QPushButton::clicked, this, [this]() {
      this->setEnabled(false);

      if (m_controller.isAcquiring()) {
        m_btnStartStopAcquisition->setText("Stopping");
        m_btnStartStopAcquisition->setStyleSheet("background-color: yellow");
        m_controller.stopAcquisition();

      } else {

        m_btnStartStopAcquisition->setText("Starting");
        QMetaObject::invokeMethod(&m_controller,
                                  &AcquisitionControllerObj::startAcquisition,
                                  m_acqParams, selectedMode());
      }
    });

    // State changed to running
    connect(&m_controller, &AcquisitionControllerObj::sigAcquisitionStarted,
            this, [this]() {
              if (m_acquiringBackground) {
                // Acquiring background
                // no op
                m_btnAcquireBackgound->setStyleSheet("background-color: red");

              } else {
                this->setEnabled(true);
                m_sbMaxFrames->setEnabled(false);
                m_btnSaveOrDisplay->setEnabled(false);

                m_btnStartStopAcquisition->setText("Stop");
                m_btnStartStopAcquisition->setStyleSheet(
                    "background-color: red");
              }
            });

    // State changed to stopped
    connect(&m_controller, &AcquisitionControllerObj::sigAcquisitionFinished,
            this, [this](const QString &filepath) {
              this->setEnabled(true);

              if (m_acquiringBackground) {
                // Acquiring background finished
                m_acquiringBackground = false;
                m_btnAcquireBackgound->setStyleSheet("");

              } else {
                m_sbMaxFrames->setEnabled(true);
                m_btnSaveOrDisplay->setEnabled(true);

                m_btnStartStopAcquisition->setText("Start");
                m_btnStartStopAcquisition->setStyleSheet(
                    "background-color: green");
              }
            });

    connect(&m_controller, &AcquisitionControllerObj::error, this,
            [this](const QString &msg) {
              QMessageBox::information(this, "Acquisition controller", msg);
            });
  }

  // Save/display button
  {
    grid->addWidget(m_btnSaveOrDisplay, row, 1);
    m_btnSaveOrDisplay->setCheckable(true);
    connect(m_btnSaveOrDisplay, &QPushButton::toggled, this,
            [this](bool checked) {
              auto *btn = m_btnSaveOrDisplay;
              if (checked) {
                btn->setText("Saving");
                btn->setStyleSheet("background-color: green");

                m_controller.daq().setSaveData(true);
              } else {
                btn->setText("Display only");
                btn->setStyleSheet("");

                m_controller.daq().setSaveData(false);
              }
            });
    m_btnSaveOrDisplay->setChecked(true);
  }
}

AcquisitionController::~AcquisitionController() {
  if (m_controllerThread.isRunning()) {
    m_controller.stopAcquisition();
    m_controllerThread.quit();
    m_controllerThread.wait();
  }
}

void AcquisitionController::closeEvent(QCloseEvent *event) {
  QWidget::closeEvent(event);
}

} // namespace OCT

#endif