#include "MainWindow.hpp"
#include "FileIO.hpp"
#include "FrameController.hpp"
#include "OCTRecon.hpp"
#include "ReconWorker.hpp"
#include "strOps.hpp"
#include "timeit.hpp"
#include <QDockWidget>
#include <QFileInfo>
#include <QLabel>
#include <QMenuBar>
#include <QMimeData>
#include <QStatusBar>
#include <QVBoxLayout>
#include <algorithm>
#include <filesystem>
#include <fmt/format.h>
#include <opencv2/opencv.hpp>

namespace OCT {

MainWindow::MainWindow()
    : m_menuFile(menuBar()->addMenu("&File")),
      m_menuView(menuBar()->addMenu("&View")), m_imageDisplay(new ImageDisplay),
      m_frameController(new FrameController) {

  // Configure MainWindow
  // --------------------
  // Enable status bar
  statusBar();

  // Enable drag and drop
  setAcceptDrops(true);

  // Define GUI
  // ----------

  // Central widget
  // --------------
  auto *centralWidget = new QWidget;
  setCentralWidget(centralWidget);
  auto *centralLayout = new QVBoxLayout;
  centralWidget->setLayout(centralLayout);
  centralLayout->addWidget(m_imageDisplay);
  {
    auto *label = new QLabel("Optical imaging!");
    centralLayout->addWidget(label);
  }

  // Dock widgets
  // ------------
  {
    auto *dock = new QDockWidget("Frames");
    this->addDockWidget(Qt::TopDockWidgetArea, dock);
    m_menuView->addAction(dock->toggleViewAction());

    dock->setWidget(m_frameController);

    connect(m_frameController, &FrameController::posChanged, this,
            &MainWindow::loadFrame);

    menuBar()->addMenu(m_frameController->menu());
  }
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

auto getDirectoryName(const fs::path &path) {
  std::string name;
  if (fs::is_directory(path)) {
    name = path.filename().string();
    if (name.empty()) {
      name = path.parent_path().filename().string();
    }
  }
  return name;
}

void MainWindow::dropEvent(QDropEvent *event) {
  const auto *mimeData = event->mimeData();
  if (mimeData->hasUrls()) {
    const auto &urls = mimeData->urls();
    const auto stdPath = QStringToStdPath(urls[0].toLocalFile());

    if (fs::is_directory(stdPath)) {
      // Check if it is a calibration directory (a directory that contains
      // "SSOCTBackground.txt" and "SSOCTCalibration180MHZ.txt")
      const auto dirName = toLower(getDirectoryName(stdPath));
      if (dirName.find("calib") != std::string::npos) {

        tryLoadCalibDirectory(stdPath);

      } else {
        // Dat sequence directory
        tryLoadDatDirectory(stdPath);
      }
    }
  }
}

void MainWindow::tryLoadCalibDirectory(const fs::path &calibDir) {
  const auto backgroundFile = calibDir / "SSOCTBackground.txt";
  const auto phaseFile = calibDir / "SSOCTCalibration180MHZ.txt";

  constexpr int statusTimeoutMs = 5000;

  if (fs::exists(backgroundFile) && fs::exists(phaseFile)) {
    m_calib = std::make_unique<Calibration<Float>>(DatReader::ALineSize,
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

void MainWindow::tryLoadDatDirectory(const fs::path &dir) {
  m_datReader = std::make_unique<DatReader>(dir);

  constexpr int statusTimeoutMs = 5000;
  if (m_datReader->ok()) {
    const auto msg = QString("Loaded dat directory ") + QString(dir.c_str());
    statusBar()->showMessage(msg, statusTimeoutMs);

    m_frameController->setSize(m_datReader->size());
    m_frameController->setPos(0);

    loadFrame(0);

  } else {
    const auto msg =
        QString("Failed to load dat directory ") + QString(dir.c_str());
    statusBar()->showMessage(msg, statusTimeoutMs);

    m_datReader = nullptr;
  }
}

void MainWindow::loadFrame(size_t i) {
  if (m_calib != nullptr) {
    // Recon 1 frame
    i = std::clamp<size_t>(i, 0, m_datReader->size());

    TimeIt timeit;

    std::vector<uint16_t> fringe(m_datReader->samplesPerFrame());
    auto err = m_datReader->read(i, 1, fringe);
    if (err) {
      auto msg = fmt::format("While loading {}/{}, got {}", i,
                             m_datReader->size(), *err);
      statusBar()->showMessage(QString::fromStdString(msg));
      return;
    }

    cv::Mat_<uint8_t> img;

    float elapsedRecon{};
    {
      TimeIt timeitRecon;
      img = reconBscan<Float>(*m_calib, fringe, m_datReader->ALineSize);
      elapsedRecon = timeitRecon.get_ms();
    }
    m_imageDisplay->imshow(matToQPixmap(img));

    auto elapsedTotal = timeit.get_ms();
    auto msg =
        fmt::format("Loaded frame {}/{}, recon {:.3f} ms, total {:.3f} ms", i,
                    m_datReader->size(), elapsedRecon, elapsedTotal);
    statusBar()->showMessage(QString::fromStdString(msg));
  }
}

} // namespace OCT