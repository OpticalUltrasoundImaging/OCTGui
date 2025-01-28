#pragma once

#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QSpinBox>
#include <QWidget>
#include <fmt/core.h>

/**
USB Serial motor controller

The 3D motor drives a metric micrometer with 0.5 mm or 500 um per revolution
The 3D motor has 1600 steps per revolution

The probe rotates at 10 rps
To achieve 50 um per frame,

(500 um/rev) / (50 um / 0.1s) = 1 s / rev

To achieve 100 um /s, or 100 per frame

Direction: Low is pull, high is push
*/
class MotorDriver : public QWidget {
  Q_OBJECT
public:
  explicit MotorDriver(QWidget *parent = nullptr)
      : QWidget(parent), m_cbPort(new QComboBox), m_btnDir(new QPushButton),
        m_btnRunStop(new QPushButton), m_sbPeriod(new QSpinBox) {

    auto *grid = new QGridLayout;
    setLayout(grid);

    int row = 0;

    {
      auto *lbl = new QLabel("Serial port:");
      grid->addWidget(lbl, row, 0);
      grid->addWidget(m_cbPort, row++, 1);
    }

    {
      auto *lbl = new QLabel("Period (us):");
      grid->addWidget(lbl, row, 0);
      grid->addWidget(m_sbPeriod, row++, 1);

      m_sbPeriod->setRange(0, 1e6);
      m_sbPeriod->setValue(m_period_us);

      connect(m_sbPeriod, &QSpinBox::valueChanged, this,
              &MotorDriver::setPeriod);
    }

    {
      grid->addWidget(m_btnDir, row, 0);
      grid->addWidget(m_btnRunStop, row++, 1);

      m_btnDir->setCheckable(true);
      connect(m_btnDir, &QPushButton::clicked, this,
              &MotorDriver::handleDirectionButton);

      m_btnRunStop->setCheckable(true);
      connect(m_btnRunStop, &QPushButton::clicked, this,
              &MotorDriver::handleRunStopButton);
    }

    // Setup serial port
    refreshPorts();
    openPort();

    m_btnDir->setChecked(false);
    m_btnRunStop->setChecked(false);
    handleDirectionButton(false);
    handleRunStopButton(false);
  };

  bool refreshPorts() {
    const auto infos = QSerialPortInfo::availablePorts();
    m_cbPort->clear();
    for (const auto &info : infos) {
      m_cbPort->addItem(info.portName());
    }
    return !infos.empty();
  }

  [[nodiscard]] bool isOpen() const { return m_port.isOpen(); }

  bool openPort() {
    if (m_port.isOpen()) {
      m_port.close();
    }
    m_portName = m_cbPort->currentText();
    m_port.setPortName(m_portName);
    m_port.setBaudRate(m_BaudRate);

    if (m_port.open(QIODevice::ReadWrite)) {
      setDirection(m_direction);
      setPeriod(m_period_us);

      setControlsEnabled(true);
      return true;
    }

    setControlsEnabled(false);
    return false;
  }

  void setControlsEnabled(bool enabled) {
    m_btnDir->setEnabled(enabled);
    m_btnRunStop->setEnabled(enabled);
  }

public Q_SLOTS:
  void handleDirectionButton(bool checked) {
    m_btnDir->setText(checked ? "Pushing" : "Pulling");
    if (m_port.isOpen()) {
      setDirection(checked);
    }
  }

  void setDirection(bool dir) {
    if (m_port.isOpen()) {
      m_direction = dir;
      m_port.write(m_direction ? "d1" : "d0");
    }
  }
  void setPeriod(int period) {
    if (m_port.isOpen()) {
      m_period_us = period;
      const auto msg = fmt::format("p{}", period);
      m_port.write(msg.c_str());
    }
  }

  void handleRunStopButton(bool checked) {
    if (checked) {
      m_btnRunStop->setText("Stop");
      run();
    } else {
      m_btnRunStop->setText("Run");
      stop();
    }
  }

  void run() {
    if (m_port.isOpen()) {
      m_running = true;
      m_port.write("r");
    }
  }

  void stop() {
    if (m_port.isOpen()) {
      m_running = false;
      m_port.write("s");
    }
  }

private:
  // Serial port
  QString m_portName;
  QSerialPort m_port;
  QSerialPort::BaudRate m_BaudRate{QSerialPort::Baud115200};

  // UI
  QComboBox *m_cbPort;
  QPushButton *m_btnDir;
  QPushButton *m_btnRunStop;
  QSpinBox *m_sbPeriod;

  // Variables
  bool m_running{false};
  bool m_direction{false};
  int m_period_us{625};
};