#include "MainWindow.hpp"
#include <QMenuBar>

namespace OCT {

MainWindow::MainWindow()
    : m_menuFile(menuBar()->addMenu("&File")),
      m_menuView(menuBar()->addMenu("&View")) {}

} // namespace OCT