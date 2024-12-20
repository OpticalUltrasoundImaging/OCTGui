#pragma once

#include "Common.hpp"
#include "FileIO.hpp"
#include "FrameController.hpp"
#include "ImageDisplay.hpp"
#include "OCTRecon.hpp"
#include "RingBuffer.hpp"
#include <QAction>
#include <QDockwidget>
#include <QDropEvent>
#include <QEvent>
#include <QMainWindow>
#include <QMenu>
#include <memory>

namespace OCT {

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  MainWindow();

protected:
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dropEvent(QDropEvent *event) override;

private:
  QMenu *m_menuFile;
  QMenu *m_menuView;

  ImageDisplay *m_imageDisplay;
  FrameController *m_frameController;

  std::unique_ptr<DatReader> m_datReader;
  std::unique_ptr<Calibration<Float>> m_calib;
  std::unique_ptr<RingBufferOfVec<uint16_t>>
      m_ringBuffer; // ring buffer for reading fringes

  void tryLoadCalibDirectory(const fs::path &calibDir);
  void tryLoadDatDirectory(const fs::path &dir);

  void loadFrame(size_t i);
};

} // namespace OCT