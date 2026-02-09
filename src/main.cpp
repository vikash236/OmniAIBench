/*
 * OmniAIBench - Professional Hardware & AI Benchmark Suite
 * Main Application Entry Point
 * License: MIT
 */

#include "ui/main_window.h"
#include <QApplication>
#include <QMessageBox>


int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  // Set application metadata
  QApplication::setApplicationName("OmniAIBench");
  QApplication::setApplicationVersion("1.0.0");
  QApplication::setOrganizationName("OmniAIBench Contributors");

  try {
    // Create and show main window
    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
  } catch (const std::exception &e) {
    QMessageBox::critical(nullptr, "Error",
                          QString("Exception: %1").arg(e.what()));
    return 1;
  }
}
