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

#ifdef OCTGUI_HAS_ALAZAR
#include "AcquisitionController.hpp"
#endif

namespace OCT {

// NOLINTNEXTLINE(*-member-functions)
class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  MainWindow();
  ~MainWindow() override;

public Q_SLOTS:
  void statusBarMessage(const QString &msg) { statusBar()->showMessage(msg); }

  void tryLoadCalibDirectory(const QString &calibDir);
  void tryLoadDatDirectory(const QString &qdir);
  void tryLoadBinfile(const QString &qpath);

  void loadFrame(size_t i);

protected:
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dropEvent(QDropEvent *event) override;

private:
  QMenu *m_menuFile;
  QMenu *m_menuView;

  ImageDisplay *m_imageDisplay;
  FrameController *m_frameController;
  OCTReconParamsController *m_reconParamsController;

#ifdef WIN32
  QString defaultDataDir{"C:/Data/"};
#else
  QString defaultDataDir{"~/data/"};
#endif
  DatFileReader m_datReader;
  std::shared_ptr<Calibration<Float>> m_calib;

  // ring buffer for reading fringes
  std::shared_ptr<RingBuffer<OCTData<Float>>> m_ringBuffer;
  ReconWorker *m_worker;
  QThread m_workerThread;

  ExportSettingsWidget *m_exportSettingsWidget;

#ifdef OCTGUI_HAS_ALAZAR
  // Acquisition
  AcquisitionController *m_acqController;
#endif

  // Called after a new DatReader is ready.
  // Updates UI elements with the new DatReader.
  void afterDatReaderReady();
};

} // namespace OCT