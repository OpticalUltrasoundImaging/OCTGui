#pragma once

#include "Common.hpp"
#include "ExportSettings.hpp"
#include "FileIO.hpp"
#include "FrameController.hpp"
#include "ImageDisplay.hpp"
#include "OCTRecon.hpp"
#include "OCTReconParamsController.hpp"
#include "ReconWorker.hpp"
#include "RingBuffer.hpp"
#include <QAction>
#include <QDockwidget>
#include <QDropEvent>
#include <QEvent>
#include <QMainWindow>
#include <QMenu>
#include <QStatusBar>
#include <QThread>
#include <memory>

namespace OCT {

// NOLINTNEXTLINE(*-member-functions)
class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  MainWindow();
  ~MainWindow() override;

public Q_SLOTS:
  void statusBarMessage(const QString &msg) { statusBar()->showMessage(msg); }

protected:
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dropEvent(QDropEvent *event) override;

private:
  QMenu *m_menuFile;
  QMenu *m_menuView;

  ImageDisplay *m_imageDisplay;
  FrameController *m_frameController;
  OCTReconParamsController *m_reconParamsController;

  std::unique_ptr<DatReader> m_datReader;
  std::shared_ptr<Calibration<Float>> m_calib;

  // ring buffer for reading fringes
  std::shared_ptr<RingBuffer<OCTData<Float>>> m_ringBuffer;
  ReconWorker *m_worker;
  QThread m_workerThread;

  ExportSettingsWidget *m_exportSettingsWidget;

  void tryLoadCalibDirectory(const QString &calibDir);
  void tryLoadDatDirectory(const QString &dir);

  void loadFrame(size_t i);
};

} // namespace OCT