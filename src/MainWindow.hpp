#pragma once

#include "FileIO.hpp"
#include "OCTRecon.hpp"
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
  using T = float;

  MainWindow();

protected:
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dropEvent(QDropEvent *event) override;

private:
  QMenu *m_menuFile;
  QMenu *m_menuView;

  std::unique_ptr<DatReader> m_datReader;
  std::unique_ptr<Calibration<T>> m_calib;

  void tryLoadCalibDirectory(const fs::path &calibDir);
};

} // namespace OCT