#include "MainWindow.hpp"
#include "FileIO.hpp"
#include "strOps.hpp"
#include <QFileInfo>
#include <QMenuBar>
#include <QMimeData>
#include <QStatusBar>
#include <filesystem>

namespace OCT {

MainWindow::MainWindow()
    : m_menuFile(menuBar()->addMenu("&File")),
      m_menuView(menuBar()->addMenu("&View")) {

  // Enable status bar
  statusBar();

  // Enable drag and drop
  setAcceptDrops(true);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
  const auto *mimeData = event->mimeData();
  if (mimeData->hasUrls()) {
    const auto &urls = mimeData->urls();

    if (urls.size() == 1) {
      // Accept folder drop
      const auto filePath = urls[0].toLocalFile();
      QFileInfo fileInfo(filePath);

      if (fileInfo.isDir()) {
        event->acceptProposedAction();
      }
    }
  }
}

void MainWindow::dropEvent(QDropEvent *event) {
  const auto *mimeData = event->mimeData();
  if (mimeData->hasUrls()) {
    const auto &urls = mimeData->urls();
    const auto stdPath = QStringToStdPath(urls[0].toLocalFile());

    if (fs::is_directory(stdPath)) {
      // Check if it is a calibration directory (a directory that contains
      // "SSOCTBackground.txt" and "SSOCTCalibration180MHZ.txt")
      const auto dirName = toLower(stdPath.stem().string());
      if (dirName.find("calib") != std::string::npos) {

        tryLoadCalibDirectory(stdPath);

      } else {
        // Dat sequence directory
        m_datReader = std::make_unique<DatReader>(stdPath);
      }
    }
  }
}

void MainWindow::tryLoadCalibDirectory(const fs::path &calibDir) {
  const auto backgroundFile = calibDir / "SSOCTBackground.txt";
  const auto phaseFile = calibDir / "SSOCTCalibration180MHZ.txt";

  constexpr int statusTimeoutMs = 5000;

  if (fs::exists(backgroundFile) && fs::exists(phaseFile)) {
    m_calib = std::make_unique<Calibration<T>>(DatReader::ALineSize,
                                               backgroundFile, phaseFile);

    const auto msg =
        QString("Loaded calibration files from ") + QString(calibDir.c_str());
    statusBar()->showMessage(msg, statusTimeoutMs);
  } else {
    // TODO more logging
    const auto msg = QString("Failed to load calibration files from ") +
                     QString(calibDir.c_str());
    statusBar()->showMessage(msg, statusTimeoutMs);
  }
}

} // namespace OCT