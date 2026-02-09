/*
 * Dashboard Widget - Main overview page combining HWiNFO64, 3DMark, and
 * Geekbench styles License: MIT
 */

#pragma once

#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include "core/hardware_detector.h"
#include "core/sensor_monitor.h"
#include <memory>

class DashboardWidget : public QWidget {
  Q_OBJECT

public:
  explicit DashboardWidget(QWidget *parent = nullptr);

public slots:
  void updateData();

private:
  void setupUI();
  void createSystemSummarySection();
  void createCPUSection();
  void createGPUSection();
  void createRAMSection();
  void createBenchmarkCardsSection();
  void applyStyles();

  // System Summary (3DMark style)
  QLabel *lblCPUModel;
  QLabel *lblGPUModel;
  QLabel *lblRAMInfo;
  QLabel *lblStorageInfo;

  // CPU Section (HWiNFO64 style)
  QLabel *lblCPUTemp;
  QLabel *lblCPUFreq;
  QLabel *lblCPUUsage;
  QLabel *lblCPUCores;
  QLabel *lblCPUThreads;
  QLabel *lblCPUPower;
  QProgressBar *cpuUsageBar;

  // GPU Section (HWiNFO64 style)
  QLabel *lblGPUTemp;
  QLabel *lblGPUUsage;
  QLabel *lblGPUVRAM;
  QLabel *lblGPUClock;
  QLabel *lblGPUPower;
  QLabel *lblGPUFanSpeed;
  QProgressBar *gpuUsageBar;
  QProgressBar *vramUsageBar;

  // RAM Section
  QLabel *lblRAMUsed;
  QLabel *lblRAMTotal;
  QLabel *lblRAMSpeed;
  QProgressBar *ramUsageBar;

  // Layouts
  QVBoxLayout *mainLayout;
  QHBoxLayout *systemSummaryLayout;
  QGridLayout *sensorsGrid;

  // Timer for updates
  QTimer *updateTimer;

  // Hardware detection
  std::unique_ptr<HardwareDetector> hwDetector;
  HardwareDetector::SystemInfo sysInfo;
  bool hardwareDetected = false;

  // Real-time sensor monitoring (NVML for GPU)
  std::unique_ptr<SensorMonitor> sensorMonitor;
  bool nvmlAvailable = false;
};
