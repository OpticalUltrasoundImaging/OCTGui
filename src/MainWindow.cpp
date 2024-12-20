#include "MainWindow.hpp"
#include "FileIO.hpp"
#include <QFileInfo>
#include <QMenuBar>
#include <QMimeData>

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

static fs::path QStringToStdPath(const QString &str) {
  const auto rawPath = str.toLocal8Bit();
  return {rawPath.begin(), rawPath.end()};
}

void MainWindow::dropEvent(QDropEvent *event) {
  const auto *mimeData = event->mimeData();
  if (mimeData->hasUrls()) {
    const auto &urls = mimeData->urls();

    const auto stdPath = QStringToStdPath(urls[0].toLocalFile());
    m_datReader = std::make_unique<DatReader>(stdPath);
  }
}

} // namespace OCT