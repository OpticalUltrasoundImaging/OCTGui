#include "MainWindow.hpp"
#include <QApplication>
#include <fmt/core.h>

int main(int argc, char *argv[]) {
  QApplication::setStyle("Fusion");
  QApplication app(argc, argv);

  OCT::MainWindow mainWindow;
  mainWindow.setWindowTitle("OCTGui");
  mainWindow.showMaximized();

  return QApplication::exec();
}
