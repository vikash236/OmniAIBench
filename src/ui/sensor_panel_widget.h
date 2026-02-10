/*
 * Sensor Panel Widget - HWiNFO64-style sensor monitoring
 * Tab 1: System Summary dashboard (vendor logos, mini charts, hw overview)
 * Tab 2: Sensor Status (left tree | right data table with Current/Min/Max/Avg)
 * License: MIT
 */

#pragma once

#include "core/hardware_detector.h"
#include "core/sensor_monitor.h"
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QScrollArea>
#include <QSplitter>
#include <QTabWidget>
#include <QTableWidget>
#include <QTimer>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <memory>

// Forward declarations for Qt Charts (included in .cpp only)
class QChartView;
class QLineSeries;

class SensorPanelWidget : public QWidget {
  Q_OBJECT

public:
  explicit SensorPanelWidget(QWidget *parent = nullptr);
  void updateData();

private:
  void setupUI();
  void applyStyles();

  // System Summary tab
  void setupSummaryTab(QWidget *tab);
  void updateSummaryTab();
  QGroupBox *createInfoGroup(const QString &title, const QColor &borderColor);
  QLabel *createKeyLabel(const QString &text);
  QLabel *createValueLabel(const QString &text);
  QLabel *createVendorLogo(const QString &vendor);

  // Mini chart helpers
  QChartView *createMiniChart(const QString &title, const QColor &lineColor);
  void updateMiniChart(QChartView *chart, QLineSeries *series, float value);

  // Sensor Status tab
  void setupSensorTab(QWidget *tab);
  void populateHardwareTree();
  void populateSensorTable(const QString &category);

  // Color helpers
  static QString tempColor(float temp);
  static QString loadColor(float load);

  // UI - Main
  QTabWidget *tabWidget = nullptr;
  QTimer *updateTimer = nullptr;

  // UI - Summary tab
  QWidget *summaryTab = nullptr;
  QLabel *cpuNameLabel = nullptr;
  QLabel *cpuCoresLabel = nullptr;
  QLabel *cpuClockLabel = nullptr;
  QLabel *cpuCacheLabel = nullptr;
  QLabel *cpuFeaturesLabel = nullptr;
  QLabel *cpuTempLabel = nullptr;
  QLabel *cpuUsageLabel = nullptr;
  QLabel *cpuPowerLabel = nullptr;
  QLabel *cpuVoltageLabel = nullptr;
  QLabel *memTotalLabel = nullptr;
  QLabel *memUsedLabel = nullptr;
  QLabel *memAvailLabel = nullptr;
  QLabel *memUsageLabel = nullptr;
  QLabel *memTypeLabel = nullptr;
  QLabel *memSpeedLabel = nullptr;
  QLabel *osLabel = nullptr;

  // Mini charts
  QChartView *cpuTempChart = nullptr;
  QLineSeries *cpuTempSeries = nullptr;
  QChartView *cpuUsageChart = nullptr;
  QLineSeries *cpuUsageSeries = nullptr;
  QChartView *gpuTempChart = nullptr;
  QLineSeries *gpuTempSeries = nullptr;
  QChartView *gpuUsageChart = nullptr;
  QLineSeries *gpuUsageSeries = nullptr;
  int chartSampleCount = 0;

  // GPU labels
  struct GPUSummaryLabels {
    QLabel *name = nullptr;
    QLabel *vram = nullptr;
    QLabel *shared = nullptr;
    QLabel *driver = nullptr;
    QLabel *temp = nullptr;
    QLabel *usage = nullptr;
    QLabel *power = nullptr;
    QLabel *clock = nullptr;
    QLabel *memClock = nullptr;
    QLabel *fanSpeed = nullptr;
    QLabel *vramUsed = nullptr;
  };
  QVector<GPUSummaryLabels> gpuLabels;

  // Storage labels
  struct StorageSummaryLabels {
    QLabel *name = nullptr;
    QLabel *type = nullptr;
    QLabel *size = nullptr;
  };
  QVector<StorageSummaryLabels> storageLabels;

  // UI - Sensor tab
  QSplitter *splitter = nullptr;
  QTreeWidget *hardwareTree = nullptr;
  QTableWidget *sensorTable = nullptr;
  QLabel *titleLabel = nullptr;
  QLabel *statusLabel = nullptr;

  // Data
  std::unique_ptr<HardwareDetector> hwDetector;
  std::unique_ptr<SensorMonitor> sensorMonitor;
  HardwareDetector::SystemInfo sysInfo;
  bool hardwareDetected = false;
  bool nvmlAvailable = false;
  QString currentCategory = "All";

  // Sensor tracking for min/max/avg
  struct SensorRecord {
    QString name;
    QString category;
    float current = 0;
    float min = 999999;
    float max = -999999;
    float sum = 0;
    int count = 0;
    QString unit;
    QString sensorType; // Temperature, Load, Clock, Power, Fan, Voltage, Data
  };
  QVector<SensorRecord> sensorRecords;

  void addOrUpdateRecord(const QString &name, const QString &category,
                         float value, const QString &unit,
                         const QString &sensorType);
  int findRecord(const QString &name, const QString &category);

private slots:
  void onTreeItemClicked(QTreeWidgetItem *item, int column);
};
