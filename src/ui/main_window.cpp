/*
 * MainWindow Implementation
 * License: MIT
 */

#include "main_window.h"
#include "cpu_benchmark_widget.h"
#include "dashboard_widget.h"
#include "gpu_benchmark_widget.h"
#include "leaderboard_widget.h"
#include "npu_benchmark_widget.h"
#include "sensor_panel_widget.h"

#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setupUI();
  applyDarkTheme();

  // Auto-refresh sensors every 5 seconds
  refreshTimer = new QTimer(this);
  connect(refreshTimer, &QTimer::timeout, this, &MainWindow::updateSensors);
  refreshTimer->start(5000);

  // MainWindow initialized
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI() {
  setWindowTitle("OmniAIBench - Professional Hardware & AI Benchmark Suite");
  setMinimumSize(1400, 900);

  // Central widget
  centralWidget = new QWidget(this);
  setCentralWidget(centralWidget);

  QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  // Header with navigation
  setupNavigationBar();

  // Stacked widget for pages
  stackedWidget = new QStackedWidget(this);

  // Create pages
  dashboardWidget = new DashboardWidget(this);
  sensorPanelWidget = new SensorPanelWidget(this);
  cpuBenchmarkWidget = new CPUBenchmarkWidget(this);
  gpuBenchmarkWidget = new GPUBenchmarkWidget(this);
  npuBenchmarkWidget = new NPUBenchmarkWidget(this);
  leaderboardWidget = new LeaderboardWidget(this);

  stackedWidget->addWidget(dashboardWidget);
  stackedWidget->addWidget(sensorPanelWidget);
  stackedWidget->addWidget(cpuBenchmarkWidget);
  stackedWidget->addWidget(gpuBenchmarkWidget);
  stackedWidget->addWidget(npuBenchmarkWidget);
  stackedWidget->addWidget(leaderboardWidget);

  mainLayout->addWidget(stackedWidget);

  // Show dashboard by default
  showDashboard();
}

void MainWindow::setupNavigationBar() {
  QWidget *navBar = new QWidget(this);
  navBar->setObjectName("navigationBar");
  navBar->setFixedHeight(60);

  QHBoxLayout *navLayout = new QHBoxLayout(navBar);
  navLayout->setContentsMargins(20, 10, 20, 10);

  // Logo/Title
  QLabel *titleLabel = new QLabel("OmniAIBench", this);
  QFont titleFont("Segoe UI", 16, QFont::Bold);
  titleLabel->setFont(titleFont);
  navLayout->addWidget(titleLabel);

  navLayout->addStretch();

  // Navigation buttons
  btnDashboard = new QPushButton("Dashboard", this);
  btnSensors = new QPushButton("Sensors", this);
  btnCPUBenchmark = new QPushButton("CPU Benchmark", this);
  btnGPUBenchmark = new QPushButton("GPU Benchmark", this);
  btnNPUBenchmark = new QPushButton("NPU Benchmark", this);
  btnLeaderboard = new QPushButton("Leaderboard", this);

  connect(btnDashboard, &QPushButton::clicked, this,
          &MainWindow::showDashboard);
  connect(btnSensors, &QPushButton::clicked, this, &MainWindow::showSensors);
  connect(btnCPUBenchmark, &QPushButton::clicked, this,
          &MainWindow::showCPUBenchmark);
  connect(btnGPUBenchmark, &QPushButton::clicked, this,
          &MainWindow::showGPUBenchmark);
  connect(btnNPUBenchmark, &QPushButton::clicked, this,
          &MainWindow::showNPUBenchmark);
  connect(btnLeaderboard, &QPushButton::clicked, this,
          &MainWindow::showLeaderboard);

  navLayout->addWidget(btnDashboard);
  navLayout->addWidget(btnSensors);
  navLayout->addWidget(btnCPUBenchmark);
  navLayout->addWidget(btnGPUBenchmark);
  navLayout->addWidget(btnNPUBenchmark);
  navLayout->addWidget(btnLeaderboard);

  qobject_cast<QVBoxLayout *>(centralWidget->layout())->insertWidget(0, navBar);
}

void MainWindow::applyDarkTheme() {
  // VS Code Dark+ theme colors
  setStyleSheet(R"(
        QMainWindow {
            background-color: #1E1E1E;
            color: #D4D4D4;
        }
        
        #navigationBar {
            background-color: #252526;
            border-bottom: 1px solid #3E3E42;
        }
        
        QPushButton {
            background-color: #3E3E42;
            color: #CCCCCC;
            border: 1px solid #3E3E42;
            border-radius: 4px;
            padding: 8px 16px;
            font-size: 13px;
            font-family: 'Segoe UI', sans-serif;
        }
        
        QPushButton:hover {
            background-color: #505050;
        }
        
        QPushButton:pressed {
            background-color: #007ACC;
        }
        
        QLabel {
            color: #D4D4D4;
            font-size: 13px;
        }
    )");
}

void MainWindow::showDashboard() {
  stackedWidget->setCurrentWidget(dashboardWidget);
}

void MainWindow::showSensors() {
  stackedWidget->setCurrentWidget(sensorPanelWidget);
}

void MainWindow::showCPUBenchmark() {
  stackedWidget->setCurrentWidget(cpuBenchmarkWidget);
}

void MainWindow::showGPUBenchmark() {
  stackedWidget->setCurrentWidget(gpuBenchmarkWidget);
}

void MainWindow::showNPUBenchmark() {
  stackedWidget->setCurrentWidget(npuBenchmarkWidget);
}

void MainWindow::showLeaderboard() {
  stackedWidget->setCurrentWidget(leaderboardWidget);
}

void MainWindow::updateSensors() {
  // Refresh sensor data on all widgets
  dashboardWidget->updateData();
  sensorPanelWidget->updateData();
}
