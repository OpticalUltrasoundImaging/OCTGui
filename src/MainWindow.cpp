#include "MainWindow.hpp"
#include <QFileInfo>
#include <QMenuBar>
#include <QMimeData>

namespace OCT {

MainWindow::MainWindow()
    : m_menuFile(menuBar()->addMenu("&File")),
      m_menuView(menuBar()->addMenu("&View")) {}

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
    const auto filePath = urls[0].toLocalFile();
    // TODO
  }
}

} // namespace OCT