#pragma once

#include "FileIO.hpp"
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

  std::unique_ptr<DatReader> m_datReader;
};

} // namespace OCT