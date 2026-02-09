/*
 * MainWindow - Primary application window
 * Qt 6 native UI with VS Code Dark+ theme
 * License: MIT
 */

#pragma once

#include <QMainWindow>
#include <QPushButton>
#include <QStackedWidget>
#include <QTimer>


// Forward declarations
class DashboardWidget;
class SensorPanelWidget;
class CPUBenchmarkWidget;
class GPUBenchmarkWidget;
class NPUBenchmarkWidget;
class LeaderboardWidget;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private slots:
  void showDashboard();
  void showSensors();
  void showCPUBenchmark();
  void showGPUBenchmark();
  void showNPUBenchmark();
  void showLeaderboard();
  void updateSensors();

private:
  void setupUI();
  void setupNavigationBar();
  void applyDarkTheme();

  // UI Components
  QWidget *centralWidget;
  QStackedWidget *stackedWidget;

  // Navigation buttons
  QPushButton *btnDashboard;
  QPushButton *btnSensors;
  QPushButton *btnCPUBenchmark;
  QPushButton *btnGPUBenchmark;
  QPushButton *btnNPUBenchmark;
  QPushButton *btnLeaderboard;

  // Pages
  DashboardWidget *dashboardWidget;
  SensorPanelWidget *sensorPanelWidget;
  CPUBenchmarkWidget *cpuBenchmarkWidget;
  GPUBenchmarkWidget *gpuBenchmarkWidget;
  NPUBenchmarkWidget *npuBenchmarkWidget;
  LeaderboardWidget *leaderboardWidget;

  // Timer for auto-refresh
  QTimer *refreshTimer;
};
