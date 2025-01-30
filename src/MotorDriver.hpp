#pragma once

#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>
#include <fmt/core.h>

/**
USB Serial motor controller

The 3D motor drives a metric micrometer with 0.5 mm or 500 um per revolution
The 3D motor has 1600 steps per revolution

The probe rotates at 10 rps
To achieve 50 um per frame,
(500 um/rev) / (50 um / 0.1s) = 1 s / rev
Set motor controller period to
(1e6 usec / rev) / (2 * 1600 steps / rev) = 312.5 us

Direction: Low is pull, high is push
*/
class MotorDriver : public QWidget {
  Q_OBJECT
public:
  explicit MotorDriver(QWidget *parent = nullptr)
      : QWidget(parent), m_cbPort(new QComboBox), m_btnDir(new QPushButton),
        m_btnRunStop(new QPushButton), m_sbPeriod(new QSpinBox) {

    auto *hlayout = new QHBoxLayout;
    setLayout(hlayout);

    auto *leftLayout = new QVBoxLayout;
    auto *rightLayout = new QVBoxLayout;
    hlayout->addLayout(leftLayout);
    hlayout->addLayout(rightLayout);

    // Serial port UI
    {
      m_gbSerialPort = new QGroupBox("Serial port");
      leftLayout->addWidget(m_gbSerialPort);
      auto *grid = new QGridLayout;
      m_gbSerialPort->setLayout(grid);

      int row = 0;

      {
        auto *lbl = new QLabel("Serial port:");
        grid->addWidget(lbl, row, 0);
        grid->addWidget(m_cbPort, row, 1);

        connect(m_cbPort, &QComboBox::currentTextChanged, this,
                [this](const QString &text) {
                  if (!text.isEmpty()) {
                    m_portName = text;
                    openPort();
                  }
                });
      }
      row++;

      {
        auto *btn = new QPushButton("Connect");
        connect(btn, &QPushButton::clicked, this, &MotorDriver::openPort);
        grid->addWidget(btn, row, 0);
      }

      {
        auto *btn = new QPushButton("Refresh ports");
        connect(btn, &QPushButton::clicked, this, &MotorDriver::refreshPorts);
        grid->addWidget(btn, row, 1);
      }
    }

    // 3D motor control UI
    {
      m_gb3DMotor = new QGroupBox("3D motor");
      // rightLayout->addWidget(m_gb3DMotor);
      hlayout->addWidget(m_gb3DMotor);
      auto *grid = new QGridLayout;
      m_gb3DMotor->setLayout(grid);
      int row = 0;

      {
        auto *lbl = new QLabel("Period (us):");
        grid->addWidget(lbl, row, 0);
        grid->addWidget(m_sbPeriod, row, 1);

        m_sbPeriod->setRange(0, 1e6); // NOLINT(*-numbers)
        m_sbPeriod->setValue(m_period_us);

        connect(m_sbPeriod, &QSpinBox::editingFinished,
                [this]() { setPeriod(m_sbPeriod->value()); });
      }

      row++;
      {
        grid->addWidget(m_btnDir, row, 0);
        grid->addWidget(m_btnRunStop, row, 1);

        m_btnDir->setCheckable(true);
        connect(m_btnDir, &QPushButton::clicked, this,
                &MotorDriver::handleDirectionButton);

        m_btnRunStop->setCheckable(true);
        connect(m_btnRunStop, &QPushButton::clicked, this,
                &MotorDriver::handleRunStopButton);
      }
    }

    // Rotary motor control UI
    {
      m_gbRotaryMotor = new QGroupBox("Rotary motor");
      // rightLayout->addWidget(m_gbRotaryMotor);
      hlayout->addWidget(m_gbRotaryMotor);
      auto *grid = new QGridLayout;
      m_gbRotaryMotor->setLayout(grid);
      int row = 0;

      {
        auto *btn = new QPushButton("Start rotation");
        grid->addWidget(btn, 0, 0);

        btn->setCheckable(true);
        connect(btn, &QPushButton::clicked, [this, btn](bool checked) {
          rotaryEnable(checked);
          if (checked) {
            btn->setText("Stop rotation");
          } else {
            btn->setText("Start rotation");
          }
        });
      }
    }

    // Refresh ports BEFORE error handler connects so no error is shown at the
    // start
    setControlsEnabled(false);
    refreshPorts(); // Port will automatically be opened here if available

    // Error handling
    {
      connect(
          this, &MotorDriver::error, this,
          [this](const QString &msg) {
            QMessageBox::information(this, "Motor driver error", msg);
          },
          Qt::QueuedConnection);
    }
  }

  bool refreshPorts() {
    const auto infos = QSerialPortInfo::availablePorts();
    m_cbPort->clear();
    for (const auto &info : infos) {
      m_cbPort->addItem(info.portName());
    }
    return !infos.empty();
  }

  [[nodiscard]] bool isOpen() const { return m_port.isOpen(); }

Q_SIGNALS:
  void error(QString msg);

public Q_SLOTS:
  /*
  Serial port
  */
  bool openPort() {
    if (m_port.isOpen()) {
      m_port.close();
    }

    m_portName = m_cbPort->currentText();
    m_port.setPortName(m_portName);
    m_port.setBaudRate(m_BaudRate);

    if (m_port.open(QIODevice::ReadWrite)) {
      const auto resp = readResp(2000);
      if (!resp.startsWith("OCT Motor Driver")) {
        Q_EMIT error("The COM port does not advertise an 'OCT Motor Driver'");
        m_port.close();
        return false;
      }

      qDebug() << "Open port: " << resp;

      handleDirectionButton(false);
      handleRunStopButton(false);
      setPeriod(m_period_us);

      setControlsEnabled(true);
      return true;
    }

    // Can't open
    QString msg = "Can't open port " + m_portName;
    Q_EMIT error(msg);

    setControlsEnabled(false);
    return false;
  }

  void setControlsEnabled(bool enabled) {
    m_gb3DMotor->setEnabled(enabled);
    m_gbRotaryMotor->setEnabled(enabled);
  }

  /*
  3D motor control
  */
  void handleDirectionButton(bool checked) {
    m_btnDir->setText(checked ? "Pushing" : "Pulling");
    if (m_port.isOpen()) {
      setDirection(checked);
    }
  }

  void setDirection(bool dir) {
    if (m_port.isOpen()) {
      m_direction = dir;
      const auto resp = writeRequest(m_direction ? "d1\n" : "d0\n");
      qDebug() << "Set direction:" << resp;
    }
  }

  void setPeriod(int period) {
    if (m_port.isOpen()) {
      m_period_us = period;
      const auto buf = fmt::format("p{}\n", period);
      const auto resp = writeRequest(buf.c_str());
      qDebug() << "Set period:" << resp;
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
      const auto resp = writeRequest("r\n");
      qDebug() << "Run:" << resp;
    }
  }

  void stop() {
    if (m_port.isOpen()) {
      m_running = false;
      const auto resp = writeRequest("s\n");
      qDebug() << "Stop:" << resp;
    }
  }

  /*
  Rotary motor control
  */
  void rotaryEnable(const bool enabled) {
    if (m_port.isOpen()) {
      m_rotaryEnabled = enabled;
      const auto resp = writeRequest(enabled ? "m0\n" : "m1\n");
      qDebug() << "rotaryEnable: " << resp;
    }
  }

private:
  // Serial port
  QString m_portName;
  QSerialPort m_port;
  QSerialPort::BaudRate m_BaudRate{QSerialPort::Baud115200};

  // UI
  QGroupBox *m_gbRotaryMotor;
  QGroupBox *m_gb3DMotor;
  QGroupBox *m_gbSerialPort;

  QComboBox *m_cbPort;
  QPushButton *m_btnDir;
  QPushButton *m_btnRunStop;
  QSpinBox *m_sbPeriod;

  // Variables for 3D motor
  bool m_running{false};
  bool m_direction{false};
  int m_period_us{625};

  // Variables for rotary motor
  bool m_rotaryEnabled{false};

  // Buffers
  QByteArray m_respData;

  QString writeRequest(const char *buf) {
    QString resp;
    if (m_port.isOpen()) {
      // Write request
      m_port.write(buf);

      if (m_port.waitForBytesWritten(10)) {
        resp = readResp();
      } else {
        Q_EMIT error("Wait write request timeout");
      }
    } else {
      if (openPort()) {
        resp = writeRequest(buf);
      } else {
        Q_EMIT error("Write request aborted because port cannot be opened.");
      }
    }
    return resp;
  }

  // Response is always terminated by double new lines
  QString readResp(int timeout_ms = 1000) {
    QString resp;
    while (true) {
      if (m_port.waitForReadyRead(timeout_ms)) {
        m_respData += m_port.readAll();

        if (m_respData.contains("\r\n\r\n")) {
          const auto endIndex = m_respData.indexOf("\r\n\r\n") + 4;
          resp = QString::fromUtf8(m_respData.left(endIndex));
          m_respData.remove(0, endIndex);
          break;
        }
      } else {
        const auto err = m_port.error();
        if (err != QSerialPort::NoError && err != QSerialPort::TimeoutError) {
          Q_EMIT error("Serial port error: " + m_port.errorString());
          break;
        }
      }
    }
    return resp;
  }
};