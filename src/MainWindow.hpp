#pragma once

#include <QDockwidget>
#include <QMainWindow>
#include <QMenu>

namespace OCT {

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  MainWindow();

private:
  QMenu *m_menuFile;
  QMenu *m_menuView;
};

} // namespace OCT