/*
 * Sensor Panel Widget - HWiNFO64-style sensor monitoring
 * Left: Hardware tree  |  Right: Sensor data table
 * License: MIT
 */

#pragma once

#include "core/hardware_detector.h"
#include "core/sensor_monitor.h"
#include <QHeaderView>
#include <QLabel>
#include <QSplitter>
#include <QTableWidget>
#include <QTimer>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <memory>

class SensorPanelWidget : public QWidget {
  Q_OBJECT

public:
  explicit SensorPanelWidget(QWidget *parent = nullptr);
  void updateData();

private:
  void setupUI();
  void applyStyles();
  void populateHardwareTree();
  void populateSensorTable(const QString &category);
  void updateSensorValues();

  // Color helpers
  static QString tempColor(float temp);
  static QString loadColor(float load);

  // UI
  QSplitter *splitter = nullptr;
  QTreeWidget *hardwareTree = nullptr;
  QTableWidget *sensorTable = nullptr;
  QLabel *titleLabel = nullptr;
  QLabel *statusLabel = nullptr;
  QTimer *updateTimer = nullptr;

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
