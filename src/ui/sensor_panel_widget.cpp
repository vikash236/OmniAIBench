/*
 * Sensor Panel Widget Implementation - HWiNFO64-style sensor monitoring
 * Tab 1: System Summary dashboard with hardware overview panels
 * Tab 2: Sensor Status with hardware tree + data table (Current/Min/Max/Avg)
 * License: MIT
 */

#include "sensor_panel_widget.h"
#include <QFont>
#include <QHBoxLayout>
#include <QPalette>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Pdh.h>
#include <Windows.h>


// ============================================================================
// Constructor
// ============================================================================
SensorPanelWidget::SensorPanelWidget(QWidget *parent) : QWidget(parent) {
  // Initialize hardware detector
  hwDetector = std::make_unique<HardwareDetector>();
  if (hwDetector->Initialize()) {
    sysInfo = hwDetector->DetectAll();
    hardwareDetected = true;
  }

  // Initialize sensor monitor
  sensorMonitor = std::make_unique<SensorMonitor>();
  nvmlAvailable = sensorMonitor->InitNVML();

  setupUI();
  applyStyles();

  // 2-second refresh for sensor readings
  updateTimer = new QTimer(this);
  connect(updateTimer, &QTimer::timeout, this, &SensorPanelWidget::updateData);
  updateTimer->start(2000);

  // Initial data
  updateData();
}

// ============================================================================
// UI Setup
// ============================================================================
void SensorPanelWidget::setupUI() {
  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  // Tab widget
  tabWidget = new QTabWidget(this);
  tabWidget->setObjectName("sensorTabs");

  // Tab 1: System Summary
  summaryTab = new QWidget();
  setupSummaryTab(summaryTab);
  tabWidget->addTab(summaryTab, "  System Summary  ");

  // Tab 2: Sensor Status
  auto *sensorTab = new QWidget();
  setupSensorTab(sensorTab);
  tabWidget->addTab(sensorTab, "  Sensor Status  ");

  mainLayout->addWidget(tabWidget);
}

// ============================================================================
// System Summary Tab
// ============================================================================
QGroupBox *SensorPanelWidget::createInfoGroup(const QString &title,
                                              const QColor &borderColor) {
  auto *group = new QGroupBox(title);
  group->setObjectName("infoGroup");
  QString borderHex = borderColor.name();
  group->setStyleSheet(QString("QGroupBox#infoGroup { "
                               "  border: 1px solid %1; "
                               "  border-radius: 3px; "
                               "  margin-top: 12px; "
                               "  padding: 8px 6px 6px 6px; "
                               "  font-size: 12px; "
                               "  font-weight: bold; "
                               "  color: %1; "
                               "  background-color: #1A1D21; "
                               "} "
                               "QGroupBox#infoGroup::title { "
                               "  subcontrol-origin: margin; "
                               "  left: 8px; "
                               "  padding: 0 4px; "
                               "  color: %1; "
                               "}")
                           .arg(borderHex));
  return group;
}

QLabel *SensorPanelWidget::createKeyLabel(const QString &text) {
  auto *label = new QLabel(text);
  label->setStyleSheet(
      "color: #808080; font-size: 11px; font-family: 'Segoe UI', sans-serif;");
  label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  return label;
}

QLabel *SensorPanelWidget::createValueLabel(const QString &text) {
  auto *label = new QLabel(text);
  label->setStyleSheet("color: #CCCCCC; font-size: 11px; font-family: "
                       "'Consolas', 'Courier New', monospace;");
  label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  label->setTextInteractionFlags(Qt::TextSelectableByMouse);
  return label;
}

void SensorPanelWidget::setupSummaryTab(QWidget *tab) {
  auto *scrollArea = new QScrollArea(tab);
  scrollArea->setWidgetResizable(true);
  scrollArea->setFrameShape(QFrame::NoFrame);
  scrollArea->setStyleSheet("background-color: #1E1E1E; border: none;");

  auto *scrollContent = new QWidget();
  scrollContent->setStyleSheet("background-color: #1E1E1E;");
  auto *mainGrid = new QGridLayout(scrollContent);
  mainGrid->setContentsMargins(8, 4, 8, 8);
  mainGrid->setSpacing(6);

  // ---- CPU Panel ----
  auto *cpuGroup = createInfoGroup("CPU", QColor("#569CD6"));
  auto *cpuLayout = new QGridLayout(cpuGroup);
  cpuLayout->setSpacing(3);
  cpuLayout->setContentsMargins(6, 4, 6, 4);

  QString cpuName = hardwareDetected ? QString::fromStdString(sysInfo.cpu.name)
                                     : "Detecting...";
  cpuLayout->addWidget(createKeyLabel("Processor"), 0, 0);
  cpuNameLabel = createValueLabel(cpuName);
  cpuNameLabel->setStyleSheet(
      "color: #4EC9B0; font-size: 12px; font-weight: bold; font-family: "
      "'Consolas', monospace;");
  cpuLayout->addWidget(cpuNameLabel, 0, 1, 1, 3);

  cpuLayout->addWidget(createKeyLabel("Cores / Threads"), 1, 0);
  cpuCoresLabel =
      createValueLabel(hardwareDetected ? QString("%1 / %2")
                                              .arg(sysInfo.cpu.physicalCores)
                                              .arg(sysInfo.cpu.logicalCores)
                                        : "-");
  cpuLayout->addWidget(cpuCoresLabel, 1, 1);

  cpuLayout->addWidget(createKeyLabel("Clock"), 1, 2);
  cpuClockLabel =
      createValueLabel(hardwareDetected ? QString("%1 MHz (Boost: %2 MHz)")
                                              .arg(sysInfo.cpu.baseClockMHz)
                                              .arg(sysInfo.cpu.maxClockMHz)
                                        : "-");
  cpuLayout->addWidget(cpuClockLabel, 1, 3);

  cpuLayout->addWidget(createKeyLabel("Cache"), 2, 0);
  cpuCacheLabel =
      createValueLabel(hardwareDetected ? QString("L2: %1 KB  |  L3: %2 MB")
                                              .arg(sysInfo.cpu.l2CacheKB)
                                              .arg(sysInfo.cpu.l3CacheMB)
                                        : "-");
  cpuLayout->addWidget(cpuCacheLabel, 2, 1);

  cpuLayout->addWidget(createKeyLabel("Features"), 2, 2);
  QStringList features;
  if (hardwareDetected) {
    if (sysInfo.cpu.hasAVX2)
      features << "AVX2";
    if (sysInfo.cpu.hasAVX512)
      features << "AVX-512";
    if (sysInfo.cpu.isAMDRyzenAI)
      features << "Ryzen AI";
    if (sysInfo.cpu.isIntelAIBoost)
      features << "AI Boost";
  }
  cpuFeaturesLabel =
      createValueLabel(features.isEmpty() ? "-" : features.join(", "));
  cpuLayout->addWidget(cpuFeaturesLabel, 2, 3);

  // CPU live data row
  cpuLayout->addWidget(createKeyLabel("Temperature"), 3, 0);
  cpuTempLabel = createValueLabel("--");
  cpuTempLabel->setStyleSheet(
      "color: #6A9955; font-size: 11px; font-family: 'Consolas', monospace;");
  cpuLayout->addWidget(cpuTempLabel, 3, 1);

  cpuLayout->addWidget(createKeyLabel("Usage"), 3, 2);
  cpuUsageLabel = createValueLabel("--");
  cpuLayout->addWidget(cpuUsageLabel, 3, 3);

  cpuLayout->addWidget(createKeyLabel("Power"), 4, 0);
  cpuPowerLabel = createValueLabel("--");
  cpuLayout->addWidget(cpuPowerLabel, 4, 1);

  cpuLayout->setColumnStretch(1, 1);
  cpuLayout->setColumnStretch(3, 1);
  mainGrid->addWidget(cpuGroup, 0, 0, 1, 2);

  // ---- GPU Panel(s) ----
  int gpuRow = 1;
  if (hardwareDetected) {
    for (size_t i = 0; i < sysInfo.gpus.size(); ++i) {
      const auto &gpu = sysInfo.gpus[i];
      QString gpuTitle = QString("GPU #%1").arg(i);
      QColor borderColor = (gpu.vendor == "NVIDIA" ||
                            gpu.name.find("NVIDIA") != std::string::npos)
                               ? QColor("#76B900")
                               : QColor("#ED1C24");

      auto *gpuGroup = createInfoGroup(gpuTitle, borderColor);
      auto *gpuLayout = new QGridLayout(gpuGroup);
      gpuLayout->setSpacing(3);
      gpuLayout->setContentsMargins(6, 4, 6, 4);

      GPUSummaryLabels labels;

      gpuLayout->addWidget(createKeyLabel("Name"), 0, 0);
      labels.name = createValueLabel(QString::fromStdString(gpu.name));
      labels.name->setStyleSheet(
          "color: #CE9178; font-size: 12px; font-weight: bold; font-family: "
          "'Consolas', monospace;");
      gpuLayout->addWidget(labels.name, 0, 1, 1, 3);

      gpuLayout->addWidget(createKeyLabel("VRAM"), 1, 0);
      float vramMB = gpu.vramBytes / (1024.0f * 1024.0f);
      float vramGB = vramMB / 1024.0f;
      labels.vram = createValueLabel(
          vramGB >= 1.0f ? QString("%1 GB").arg(vramGB, 0, 'f', 1)
                         : QString("%1 MB").arg(vramMB, 0, 'f', 0));
      gpuLayout->addWidget(labels.vram, 1, 1);

      gpuLayout->addWidget(createKeyLabel("Shared Memory"), 1, 2);
      float sharedMB = gpu.sharedMemBytes / (1024.0f * 1024.0f);
      float sharedGB = sharedMB / 1024.0f;
      labels.shared = createValueLabel(
          sharedGB >= 1.0f ? QString("%1 GB").arg(sharedGB, 0, 'f', 1)
                           : QString("%1 MB").arg(sharedMB, 0, 'f', 0));
      gpuLayout->addWidget(labels.shared, 1, 3);

      gpuLayout->addWidget(createKeyLabel("Driver"), 2, 0);
      labels.driver =
          createValueLabel(QString::fromStdString(gpu.driverVersion));
      gpuLayout->addWidget(labels.driver, 2, 1);

      // Live data
      gpuLayout->addWidget(createKeyLabel("Temperature"), 2, 2);
      labels.temp = createValueLabel("--");
      gpuLayout->addWidget(labels.temp, 2, 3);

      gpuLayout->addWidget(createKeyLabel("Usage"), 3, 0);
      labels.usage = createValueLabel("--");
      gpuLayout->addWidget(labels.usage, 3, 1);

      gpuLayout->addWidget(createKeyLabel("Power"), 3, 2);
      labels.power = createValueLabel("--");
      gpuLayout->addWidget(labels.power, 3, 3);

      gpuLayout->addWidget(createKeyLabel("Core Clock"), 4, 0);
      labels.clock = createValueLabel("--");
      gpuLayout->addWidget(labels.clock, 4, 1);

      gpuLayout->setColumnStretch(1, 1);
      gpuLayout->setColumnStretch(3, 1);

      int col = (int)(i % 2);
      mainGrid->addWidget(gpuGroup, gpuRow, col);
      if (col == 1 || i == sysInfo.gpus.size() - 1)
        gpuRow++;

      gpuLabels.append(labels);
    }
  }

  // ---- Memory Panel ----
  auto *memGroup = createInfoGroup("Memory", QColor("#6A9955"));
  auto *memLayout = new QGridLayout(memGroup);
  memLayout->setSpacing(3);
  memLayout->setContentsMargins(6, 4, 6, 4);

  memLayout->addWidget(createKeyLabel("Total"), 0, 0);
  memTotalLabel = createValueLabel("--");
  memTotalLabel->setStyleSheet(
      "color: #4EC9B0; font-size: 12px; font-weight: bold; font-family: "
      "'Consolas', monospace;");
  memLayout->addWidget(memTotalLabel, 0, 1);

  memLayout->addWidget(createKeyLabel("Type"), 0, 2);
  memTypeLabel = createValueLabel(hardwareDetected && !sysInfo.ram.type.empty()
                                      ? QString::fromStdString(sysInfo.ram.type)
                                      : "-");
  memLayout->addWidget(memTypeLabel, 0, 3);

  memLayout->addWidget(createKeyLabel("Used"), 1, 0);
  memUsedLabel = createValueLabel("--");
  memLayout->addWidget(memUsedLabel, 1, 1);

  memLayout->addWidget(createKeyLabel("Speed"), 1, 2);
  memSpeedLabel =
      createValueLabel(hardwareDetected && sysInfo.ram.speedMHz > 0
                           ? QString("%1 MHz").arg(sysInfo.ram.speedMHz)
                           : "-");
  memLayout->addWidget(memSpeedLabel, 1, 3);

  memLayout->addWidget(createKeyLabel("Available"), 2, 0);
  memAvailLabel = createValueLabel("--");
  memLayout->addWidget(memAvailLabel, 2, 1);

  memLayout->addWidget(createKeyLabel("Usage"), 2, 2);
  memUsageLabel = createValueLabel("--");
  memLayout->addWidget(memUsageLabel, 2, 3);

  memLayout->setColumnStretch(1, 1);
  memLayout->setColumnStretch(3, 1);
  mainGrid->addWidget(memGroup, gpuRow, 0);

  // ---- OS / System Panel ----
  auto *osGroup = createInfoGroup("Operating System", QColor("#808080"));
  auto *osLayout = new QGridLayout(osGroup);
  osLayout->setSpacing(3);
  osLayout->setContentsMargins(6, 4, 6, 4);

  osLayout->addWidget(createKeyLabel("OS"), 0, 0);
  osLabel = createValueLabel(
      hardwareDetected ? QString::fromStdString(sysInfo.osVersion) : "-");
  osLayout->addWidget(osLabel, 0, 1);
  osLayout->setColumnStretch(1, 1);
  mainGrid->addWidget(osGroup, gpuRow, 1);

  gpuRow++;

  // ---- Storage Panel ----
  if (hardwareDetected && !sysInfo.storage.empty()) {
    auto *storageGroup = createInfoGroup("Drives", QColor("#D7BA7D"));
    auto *storLayout = new QGridLayout(storageGroup);
    storLayout->setSpacing(3);
    storLayout->setContentsMargins(6, 4, 6, 4);

    // Header row
    auto *hdrName = createKeyLabel("Name");
    hdrName->setStyleSheet(
        "color: #D7BA7D; font-size: 11px; font-weight: bold;");
    auto *hdrType = createKeyLabel("Interface");
    hdrType->setStyleSheet(
        "color: #D7BA7D; font-size: 11px; font-weight: bold;");
    auto *hdrSize = createKeyLabel("Capacity");
    hdrSize->setStyleSheet(
        "color: #D7BA7D; font-size: 11px; font-weight: bold;");
    storLayout->addWidget(hdrName, 0, 0);
    storLayout->addWidget(hdrType, 0, 1);
    storLayout->addWidget(hdrSize, 0, 2);

    for (size_t i = 0; i < sysInfo.storage.size(); ++i) {
      const auto &drive = sysInfo.storage[i];
      StorageSummaryLabels labels;

      labels.name = createValueLabel(QString::fromStdString(drive.name));
      storLayout->addWidget(labels.name, (int)i + 1, 0);

      labels.type =
          createValueLabel(QString::fromStdString(drive.interfaceType) + " " +
                           QString::fromStdString(drive.type));
      storLayout->addWidget(labels.type, (int)i + 1, 1);

      float sizeGB = drive.sizeBytes / (1024.0f * 1024.0f * 1024.0f);
      labels.size = createValueLabel(
          sizeGB >= 1000 ? QString("%1 TB").arg(sizeGB / 1024.0f, 0, 'f', 2)
                         : QString("%1 GB").arg(sizeGB, 0, 'f', 1));
      storLayout->addWidget(labels.size, (int)i + 1, 2);

      storageLabels.append(labels);
    }

    storLayout->setColumnStretch(0, 2);
    storLayout->setColumnStretch(1, 1);
    storLayout->setColumnStretch(2, 1);
    mainGrid->addWidget(storageGroup, gpuRow, 0, 1, 2);
  }

  mainGrid->setRowStretch(gpuRow + 1, 1); // Push everything up
  scrollArea->setWidget(scrollContent);

  auto *tabLayout = new QVBoxLayout(tab);
  tabLayout->setContentsMargins(0, 0, 0, 0);
  tabLayout->addWidget(scrollArea);
}

// ============================================================================
// Sensor Status Tab
// ============================================================================
void SensorPanelWidget::setupSensorTab(QWidget *tab) {
  auto *layout = new QVBoxLayout(tab);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  // Top bar with status
  auto *topBar = new QWidget(tab);
  topBar->setObjectName("sensorTopBar");
  topBar->setFixedHeight(28);
  auto *topLayout = new QHBoxLayout(topBar);
  topLayout->setContentsMargins(10, 2, 10, 2);

  titleLabel = new QLabel("Sensor Status", tab);
  titleLabel->setObjectName("sensorTitle");

  statusLabel = new QLabel("Updating...", tab);
  statusLabel->setObjectName("sensorStatus");

  topLayout->addWidget(titleLabel);
  topLayout->addStretch();
  topLayout->addWidget(statusLabel);
  layout->addWidget(topBar);

  // Splitter: left tree | right table
  splitter = new QSplitter(Qt::Horizontal, tab);

  // Left: Hardware tree
  hardwareTree = new QTreeWidget(tab);
  hardwareTree->setHeaderLabel("Hardware");
  hardwareTree->setMinimumWidth(260);
  hardwareTree->setMaximumWidth(340);
  hardwareTree->setIndentation(16);
  hardwareTree->setAnimated(true);
  hardwareTree->setRootIsDecorated(true);
  connect(hardwareTree, &QTreeWidget::itemClicked, this,
          &SensorPanelWidget::onTreeItemClicked);

  // Right: Sensor data table
  sensorTable = new QTableWidget(tab);
  sensorTable->setColumnCount(5);
  sensorTable->setHorizontalHeaderLabels(
      {"Sensor", "Current", "Minimum", "Maximum", "Average"});
  sensorTable->horizontalHeader()->setStretchLastSection(true);
  sensorTable->horizontalHeader()->setSectionResizeMode(0,
                                                        QHeaderView::Stretch);
  sensorTable->horizontalHeader()->setSectionResizeMode(
      1, QHeaderView::ResizeToContents);
  sensorTable->horizontalHeader()->setSectionResizeMode(
      2, QHeaderView::ResizeToContents);
  sensorTable->horizontalHeader()->setSectionResizeMode(
      3, QHeaderView::ResizeToContents);
  sensorTable->horizontalHeader()->setSectionResizeMode(
      4, QHeaderView::ResizeToContents);
  sensorTable->verticalHeader()->setVisible(false);
  sensorTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  sensorTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  sensorTable->setAlternatingRowColors(true);
  sensorTable->setShowGrid(false);
  sensorTable->verticalHeader()->setDefaultSectionSize(20);

  splitter->addWidget(hardwareTree);
  splitter->addWidget(sensorTable);
  splitter->setSizes({260, 900});

  layout->addWidget(splitter);

  populateHardwareTree();
}

// ============================================================================
// Styles
// ============================================================================
void SensorPanelWidget::applyStyles() {
  setStyleSheet(R"(
    /* Tab Widget */
    #sensorTabs {
        background-color: #1E1E1E;
        border: none;
    }
    QTabWidget::pane {
        border: none;
        background-color: #1E1E1E;
    }
    QTabBar::tab {
        background-color: #2D2D30;
        color: #808080;
        border: none;
        border-bottom: 2px solid transparent;
        padding: 6px 16px;
        font-size: 11px;
        font-family: 'Segoe UI', sans-serif;
        font-weight: bold;
        min-width: 120px;
    }
    QTabBar::tab:selected {
        color: #FFFFFF;
        background-color: #1E1E1E;
        border-bottom: 2px solid #0078D4;
    }
    QTabBar::tab:hover:!selected {
        color: #CCCCCC;
        background-color: #252526;
    }

    /* Top bar */
    #sensorTopBar {
        background-color: #252526;
        border-bottom: 1px solid #3E3E42;
    }
    #sensorTitle {
        color: #CCCCCC;
        font-size: 12px;
        font-weight: bold;
        font-family: 'Segoe UI', sans-serif;
    }
    #sensorStatus {
        color: #6A9955;
        font-size: 10px;
        font-family: 'Segoe UI', sans-serif;
    }

    /* Hardware Tree */
    QTreeWidget {
        background-color: #1E1E1E;
        color: #CCCCCC;
        border: none;
        border-right: 1px solid #3E3E42;
        font-size: 11px;
        font-family: 'Segoe UI', sans-serif;
        outline: none;
    }
    QTreeWidget::item {
        padding: 2px 6px;
        border: none;
    }
    QTreeWidget::item:selected {
        background-color: #094771;
        color: #FFFFFF;
    }
    QTreeWidget::item:hover {
        background-color: #2A2D2E;
    }
    QTreeWidget::branch {
        background-color: #1E1E1E;
    }
    QHeaderView::section {
        background-color: #252526;
        color: #CCCCCC;
        border: none;
        border-bottom: 1px solid #3E3E42;
        padding: 3px 8px;
        font-size: 10px;
        font-weight: bold;
    }

    /* Sensor Table */
    QTableWidget {
        background-color: #1E1E1E;
        color: #CCCCCC;
        border: none;
        gridline-color: #2D2D30;
        font-size: 11px;
        font-family: 'Consolas', 'Courier New', monospace;
        outline: none;
        selection-background-color: #094771;
    }
    QTableWidget::item {
        padding: 1px 8px;
        border-bottom: 1px solid #252526;
    }
    QTableWidget::item:selected {
        background-color: #094771;
        color: #FFFFFF;
    }
    QTableWidget::item:alternate {
        background-color: #1A1A1A;
    }

    /* Scrollbars */
    QScrollBar:vertical {
        background-color: #1E1E1E;
        width: 8px;
        border: none;
    }
    QScrollBar::handle:vertical {
        background-color: #424242;
        border-radius: 4px;
        min-height: 20px;
    }
    QScrollBar::handle:vertical:hover {
        background-color: #4F4F4F;
    }
    QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
        height: 0px;
    }
    QScrollBar:horizontal {
        background-color: #1E1E1E;
        height: 8px;
        border: none;
    }
    QScrollBar::handle:horizontal {
        background-color: #424242;
        border-radius: 4px;
        min-width: 20px;
    }

    /* Splitter */
    QSplitter::handle {
        background-color: #3E3E42;
        width: 1px;
    }
  )");
}

// ============================================================================
// Hardware Tree
// ============================================================================
void SensorPanelWidget::populateHardwareTree() {
  hardwareTree->clear();

  // System root
  auto *systemItem = new QTreeWidgetItem(hardwareTree);
  QString sysName =
      hardwareDetected ? QString::fromStdString(sysInfo.cpu.name) : "System";
  systemItem->setText(0, "System: " + sysName);
  systemItem->setData(0, Qt::UserRole, "All");
  systemItem->setForeground(0, QColor("#4EC9B0"));
  systemItem->setExpanded(true);

  // CPU
  auto *cpuItem = new QTreeWidgetItem(systemItem);
  QString cpuName =
      hardwareDetected ? QString::fromStdString(sysInfo.cpu.name) : "CPU";
  cpuItem->setText(0, "CPU: " + cpuName);
  cpuItem->setData(0, Qt::UserRole, "CPU");
  cpuItem->setForeground(0, QColor("#569CD6"));
  cpuItem->setExpanded(true);

  // CPU sub-items
  auto addSubItem = [](QTreeWidgetItem *parent, const QString &text,
                       const QString &cat, const QColor &color) {
    auto *item = new QTreeWidgetItem(parent);
    item->setText(0, text);
    item->setData(0, Qt::UserRole, cat);
    item->setForeground(0, color);
  };

  addSubItem(cpuItem, "Temperatures", "CPU_Temp", QColor("#CE9178"));
  addSubItem(cpuItem, "Clocks", "CPU_Clock", QColor("#4EC9B0"));
  addSubItem(cpuItem, "Utilization", "CPU_Load", QColor("#DCDCAA"));
  addSubItem(cpuItem, "Power", "CPU_Power", QColor("#D7BA7D"));

  // GPUs
  if (hardwareDetected && !sysInfo.gpus.empty()) {
    for (size_t i = 0; i < sysInfo.gpus.size(); ++i) {
      auto *gpuItem = new QTreeWidgetItem(systemItem);
      gpuItem->setText(0,
                       "GPU: " + QString::fromStdString(sysInfo.gpus[i].name));
      gpuItem->setData(0, Qt::UserRole, QString("GPU_%1").arg(i));
      gpuItem->setForeground(0, QColor("#CE9178"));
      gpuItem->setExpanded(true);

      QString prefix = QString("GPU_%1_").arg(i);
      addSubItem(gpuItem, "Temperatures", prefix + "Temp", QColor("#CE9178"));
      addSubItem(gpuItem, "Clocks", prefix + "Clock", QColor("#4EC9B0"));
      addSubItem(gpuItem, "Utilization", prefix + "Load", QColor("#DCDCAA"));
      addSubItem(gpuItem, "Power", prefix + "Power", QColor("#D7BA7D"));
      addSubItem(gpuItem, "Fans", prefix + "Fan", QColor("#608B4E"));
      addSubItem(gpuItem, "Memory", prefix + "Mem", QColor("#569CD6"));
    }
  }

  // Memory
  auto *memItem = new QTreeWidgetItem(systemItem);
  memItem->setText(0, "Memory");
  memItem->setData(0, Qt::UserRole, "RAM");
  memItem->setForeground(0, QColor("#6A9955"));

  // Storage
  if (hardwareDetected && !sysInfo.storage.empty()) {
    auto *storageItem = new QTreeWidgetItem(systemItem);
    storageItem->setText(0, "Storage");
    storageItem->setData(0, Qt::UserRole, "Storage");
    storageItem->setForeground(0, QColor("#D7BA7D"));

    for (size_t i = 0; i < sysInfo.storage.size(); ++i) {
      addSubItem(storageItem, QString::fromStdString(sysInfo.storage[i].name),
                 QString("Storage_%1").arg(i), QColor("#808080"));
    }
  }

  hardwareTree->setCurrentItem(systemItem);
}

// ============================================================================
// Tree Item Click
// ============================================================================
void SensorPanelWidget::onTreeItemClicked(QTreeWidgetItem *item, int column) {
  Q_UNUSED(column);
  currentCategory = item->data(0, Qt::UserRole).toString();
  populateSensorTable(currentCategory);
}

// ============================================================================
// Sensor Table Population
// ============================================================================
void SensorPanelWidget::populateSensorTable(const QString &category) {
  sensorTable->setRowCount(0);

  // Group records by their category for section headers
  QString lastSection;

  for (int i = 0; i < sensorRecords.size(); ++i) {
    const auto &rec = sensorRecords[i];

    // Filter by category
    bool show = false;
    if (category == "All") {
      show = true;
    } else if (category.startsWith("CPU") && rec.category.startsWith("CPU")) {
      if (category == "CPU")
        show = true;
      else if (category == "CPU_Temp" && rec.sensorType == "Temperature")
        show = true;
      else if (category == "CPU_Clock" && rec.sensorType == "Clock")
        show = true;
      else if (category == "CPU_Load" && rec.sensorType == "Load")
        show = true;
      else if (category == "CPU_Power" &&
               (rec.sensorType == "Power" || rec.sensorType == "Voltage"))
        show = true;
    } else if (category.startsWith("GPU") && rec.category.startsWith("GPU")) {
      if (category.startsWith("GPU_0") || category.startsWith("GPU_1")) {
        QString gpuPrefix = category.left(5);
        if (!rec.category.startsWith(gpuPrefix))
          show = false;
        else if (category.endsWith("_Temp") && rec.sensorType == "Temperature")
          show = true;
        else if (category.endsWith("_Clock") && rec.sensorType == "Clock")
          show = true;
        else if (category.endsWith("_Load") && rec.sensorType == "Load")
          show = true;
        else if (category.endsWith("_Power") && rec.sensorType == "Power")
          show = true;
        else if (category.endsWith("_Fan") && rec.sensorType == "Fan")
          show = true;
        else if (category.endsWith("_Mem") && rec.sensorType == "Data")
          show = true;
        else if (!category.contains("_", Qt::CaseInsensitive) ||
                 category.count('_') == 1)
          show = true;
      } else {
        show = true;
      }
    } else if (category == "RAM" && rec.category == "RAM") {
      show = true;
    } else if (category.startsWith("Storage") &&
               rec.category.startsWith("Storage")) {
      show = true;
    }

    if (!show)
      continue;

    // Section header for "All" view
    if (category == "All" && rec.category != lastSection) {
      lastSection = rec.category;
      int headerRow = sensorTable->rowCount();
      sensorTable->insertRow(headerRow);

      // Determine section display name
      QString sectionName = rec.category;
      if (rec.category == "CPU" && hardwareDetected)
        sectionName = "CPU: " + QString::fromStdString(sysInfo.cpu.name);
      else if (rec.category.startsWith("GPU_") && hardwareDetected) {
        int idx = rec.category.mid(4).toInt();
        if (idx < (int)sysInfo.gpus.size())
          sectionName =
              "GPU: " + QString::fromStdString(sysInfo.gpus[idx].name);
      }

      auto *headerItem =
          new QTableWidgetItem(QString("  ▼ %1").arg(sectionName));
      headerItem->setForeground(QColor("#4EC9B0"));
      headerItem->setBackground(QColor("#252526"));
      QFont headerFont("Segoe UI", 10, QFont::Bold);
      headerItem->setFont(headerFont);
      sensorTable->setItem(headerRow, 0, headerItem);
      for (int c = 1; c < 5; ++c) {
        auto *spacer = new QTableWidgetItem("");
        spacer->setBackground(QColor("#252526"));
        sensorTable->setItem(headerRow, c, spacer);
      }
    }

    int row = sensorTable->rowCount();
    sensorTable->insertRow(row);

    // Sensor name with icon prefix based on type
    QString icon;
    if (rec.sensorType == "Temperature")
      icon = "🌡 ";
    else if (rec.sensorType == "Clock")
      icon = "⏱ ";
    else if (rec.sensorType == "Load")
      icon = "📊 ";
    else if (rec.sensorType == "Power")
      icon = "⚡ ";
    else if (rec.sensorType == "Fan")
      icon = "🌀 ";
    else if (rec.sensorType == "Voltage")
      icon = "🔌 ";

    auto *nameItem = new QTableWidgetItem(icon + rec.name);
    nameItem->setForeground(QColor("#CCCCCC"));
    sensorTable->setItem(row, 0, nameItem);

    // Decimal precision
    int decimals = 1;
    if (rec.unit == "MHz" || rec.unit == "RPM" || rec.unit == "MB")
      decimals = 0;
    else if (rec.unit == "W" || rec.unit == "V")
      decimals = 2;
    else if (rec.unit == "GB")
      decimals = 2;

    // Current value
    QString currentStr =
        QString("%1 %2").arg(rec.current, 0, 'f', decimals).arg(rec.unit);
    auto *currentItem = new QTableWidgetItem(currentStr);
    if (rec.sensorType == "Temperature")
      currentItem->setForeground(QColor(tempColor(rec.current)));
    else if (rec.sensorType == "Load")
      currentItem->setForeground(QColor(loadColor(rec.current)));
    else if (rec.sensorType == "Clock")
      currentItem->setForeground(QColor("#4EC9B0"));
    else if (rec.sensorType == "Power")
      currentItem->setForeground(QColor("#D7BA7D"));
    else if (rec.sensorType == "Fan")
      currentItem->setForeground(QColor("#608B4E"));
    else if (rec.sensorType == "Voltage")
      currentItem->setForeground(QColor("#B5CEA8"));
    else
      currentItem->setForeground(QColor("#DCDCAA"));
    currentItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    sensorTable->setItem(row, 1, currentItem);

    // Min
    float minVal = rec.min < 999999 ? rec.min : 0;
    auto *minItem = new QTableWidgetItem(
        QString("%1 %2").arg(minVal, 0, 'f', decimals).arg(rec.unit));
    minItem->setForeground(QColor("#569CD6"));
    minItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    sensorTable->setItem(row, 2, minItem);

    // Max
    float maxVal = rec.max > -999999 ? rec.max : 0;
    auto *maxItem = new QTableWidgetItem(
        QString("%1 %2").arg(maxVal, 0, 'f', decimals).arg(rec.unit));
    maxItem->setForeground(QColor("#CE9178"));
    maxItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    sensorTable->setItem(row, 3, maxItem);

    // Average
    float avgVal = rec.count > 0 ? rec.sum / rec.count : 0;
    auto *avgItem = new QTableWidgetItem(
        QString("%1 %2").arg(avgVal, 0, 'f', decimals).arg(rec.unit));
    avgItem->setForeground(QColor("#808080"));
    avgItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    sensorTable->setItem(row, 4, avgItem);
  }

  statusLabel->setText(
      QString("%1 sensors | Updated").arg(sensorRecords.size()));
}

// ============================================================================
// Update Summary Tab (live data)
// ============================================================================
void SensorPanelWidget::updateSummaryTab() {
  // Memory data
  MEMORYSTATUSEX memStatus;
  memStatus.dwLength = sizeof(memStatus);
  if (GlobalMemoryStatusEx(&memStatus)) {
    float totalGB =
        (float)memStatus.ullTotalPhys / (1024.0f * 1024.0f * 1024.0f);
    float usedGB = (float)(memStatus.ullTotalPhys - memStatus.ullAvailPhys) /
                   (1024.0f * 1024.0f * 1024.0f);
    float availGB =
        (float)memStatus.ullAvailPhys / (1024.0f * 1024.0f * 1024.0f);

    memTotalLabel->setText(QString("%1 GB").arg(totalGB, 0, 'f', 2));
    memUsedLabel->setText(QString("%1 GB").arg(usedGB, 0, 'f', 2));
    memAvailLabel->setText(QString("%1 GB").arg(availGB, 0, 'f', 2));
    memUsageLabel->setText(QString("%1 %").arg(memStatus.dwMemoryLoad));

    // Color usage
    float usage = (float)memStatus.dwMemoryLoad;
    memUsageLabel->setStyleSheet(
        QString("color: %1; font-size: 11px; font-family: 'Consolas', "
                "monospace;")
            .arg(loadColor(usage)));
  }

  // CPU live data from sensor records
  for (const auto &rec : sensorRecords) {
    if (rec.category == "CPU") {
      if (rec.name == "CPU Package" && rec.sensorType == "Temperature") {
        cpuTempLabel->setText(QString("%1 °C").arg(rec.current, 0, 'f', 1));
        cpuTempLabel->setStyleSheet(
            QString("color: %1; font-size: 11px; font-family: 'Consolas', "
                    "monospace;")
                .arg(tempColor(rec.current)));
      } else if (rec.name == "Total CPU Usage" && rec.sensorType == "Load") {
        cpuUsageLabel->setText(QString("%1 %").arg(rec.current, 0, 'f', 1));
        cpuUsageLabel->setStyleSheet(
            QString("color: %1; font-size: 11px; font-family: 'Consolas', "
                    "monospace;")
                .arg(loadColor(rec.current)));
      } else if (rec.name == "Package Power" && rec.sensorType == "Power") {
        cpuPowerLabel->setText(QString("%1 W").arg(rec.current, 0, 'f', 2));
      }
    }
  }

  // GPU live data
  for (int g = 0; g < gpuLabels.size(); ++g) {
    auto &labels = gpuLabels[g];
    QString gpuCat = QString("GPU_%1").arg(g);

    for (const auto &rec : sensorRecords) {
      if (rec.category != gpuCat)
        continue;

      if (rec.name == "GPU Temperature") {
        labels.temp->setText(QString("%1 °C").arg(rec.current, 0, 'f', 1));
        labels.temp->setStyleSheet(
            QString("color: %1; font-size: 11px; font-family: 'Consolas', "
                    "monospace;")
                .arg(tempColor(rec.current)));
      } else if (rec.name == "GPU Usage" || rec.name == "GPU Utilization") {
        labels.usage->setText(QString("%1 %").arg(rec.current, 0, 'f', 1));
        labels.usage->setStyleSheet(
            QString("color: %1; font-size: 11px; font-family: 'Consolas', "
                    "monospace;")
                .arg(loadColor(rec.current)));
      } else if (rec.name == "GPU Power") {
        labels.power->setText(QString("%1 W").arg(rec.current, 0, 'f', 2));
      } else if (rec.name == "GPU Core Clock") {
        labels.clock->setText(QString("%1 MHz").arg(rec.current, 0, 'f', 0));
      }
    }
  }
}

// ============================================================================
// Main Update
// ============================================================================
void SensorPanelWidget::updateData() {
  // === CPU Sensors ===
  if (sensorMonitor) {
    auto cpuSensors = sensorMonitor->ReadCPUSensorsWMI();

    if (cpuSensors.packageTemp > 0) {
      addOrUpdateRecord("CPU Package", "CPU", cpuSensors.packageTemp, "°C",
                        "Temperature");
    }
    if (cpuSensors.packagePowerW > 0) {
      addOrUpdateRecord("Package Power", "CPU", cpuSensors.packagePowerW, "W",
                        "Power");
    }
  }

  // CPU usage via Windows API
  FILETIME idleTime, kernelTime, userTime;
  static uint64_t prevIdle = 0, prevKernel = 0, prevUser = 0;

  if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
    uint64_t idle =
        ((uint64_t)idleTime.dwHighDateTime << 32) | idleTime.dwLowDateTime;
    uint64_t kernel =
        ((uint64_t)kernelTime.dwHighDateTime << 32) | kernelTime.dwLowDateTime;
    uint64_t user =
        ((uint64_t)userTime.dwHighDateTime << 32) | userTime.dwLowDateTime;

    if (prevKernel > 0) {
      uint64_t totalDiff = (kernel - prevKernel) + (user - prevUser);
      uint64_t idleDiff = idle - prevIdle;
      if (totalDiff > 0) {
        float cpuUsage =
            (float)(100.0 * (1.0 - (double)idleDiff / (double)totalDiff));
        addOrUpdateRecord("Total CPU Usage", "CPU", cpuUsage, "%", "Load");
      }
    }
    prevIdle = idle;
    prevKernel = kernel;
    prevUser = user;
  }

  // CPU frequency
  if (hardwareDetected && sysInfo.cpu.maxClockMHz > 0) {
    addOrUpdateRecord("CPU Clock", "CPU", (float)sysInfo.cpu.maxClockMHz, "MHz",
                      "Clock");
  }

  // === GPU Sensors ===
  int nvidiaGpuIdx = -1;
  int amdGpuIdx = -1;
  if (hardwareDetected) {
    for (size_t i = 0; i < sysInfo.gpus.size(); ++i) {
      if (sysInfo.gpus[i].vendor == "NVIDIA" ||
          sysInfo.gpus[i].name.find("NVIDIA") != std::string::npos ||
          sysInfo.gpus[i].name.find("GeForce") != std::string::npos) {
        nvidiaGpuIdx = (int)i;
      } else if (sysInfo.gpus[i].vendor == "AMD" ||
                 sysInfo.gpus[i].name.find("AMD") != std::string::npos ||
                 sysInfo.gpus[i].name.find("Radeon") != std::string::npos) {
        amdGpuIdx = (int)i;
      }
    }
  }

  // NVIDIA GPU via NVML
  if (nvmlAvailable && sensorMonitor && nvidiaGpuIdx >= 0) {
    auto gpuSensors = sensorMonitor->ReadNVIDIASensors();
    QString gpuCat = QString("GPU_%1").arg(nvidiaGpuIdx);

    if (gpuSensors.temperature > 0)
      addOrUpdateRecord("GPU Temperature", gpuCat, gpuSensors.temperature, "°C",
                        "Temperature");
    if (gpuSensors.gpuClock > 0)
      addOrUpdateRecord("GPU Core Clock", gpuCat, gpuSensors.gpuClock, "MHz",
                        "Clock");
    if (gpuSensors.memoryClock > 0)
      addOrUpdateRecord("GPU Memory Clock", gpuCat, gpuSensors.memoryClock,
                        "MHz", "Clock");
    if (gpuSensors.powerW > 0)
      addOrUpdateRecord("GPU Power", gpuCat, gpuSensors.powerW, "W", "Power");
    if (gpuSensors.fanSpeedPercent > 0)
      addOrUpdateRecord("GPU Fan", gpuCat, gpuSensors.fanSpeedPercent, "%",
                        "Fan");
    if (gpuSensors.fanSpeedRPM > 0)
      addOrUpdateRecord("GPU Fan Speed", gpuCat, gpuSensors.fanSpeedRPM, "RPM",
                        "Fan");
    addOrUpdateRecord("GPU Usage", gpuCat, gpuSensors.usagePercent, "%",
                      "Load");

    if (gpuSensors.memoryTotalMB > 0) {
      addOrUpdateRecord("VRAM Used", gpuCat, gpuSensors.memoryUsedMB, "MB",
                        "Data");
      addOrUpdateRecord("VRAM Total", gpuCat, gpuSensors.memoryTotalMB, "MB",
                        "Data");
      float vramPercent =
          (gpuSensors.memoryUsedMB / gpuSensors.memoryTotalMB) * 100.0f;
      addOrUpdateRecord("VRAM Usage", gpuCat, vramPercent, "%", "Load");
    }
  }

  // AMD iGPU
  if (amdGpuIdx >= 0 && hardwareDetected &&
      amdGpuIdx < (int)sysInfo.gpus.size()) {
    QString gpuCat = QString("GPU_%1").arg(amdGpuIdx);

    float detectedVramMB =
        sysInfo.gpus[amdGpuIdx].vramBytes / (1024.0f * 1024.0f);
    if (detectedVramMB > 0) {
      addOrUpdateRecord("Dedicated VRAM", gpuCat, detectedVramMB, "MB", "Data");
    }
    float sharedMemMB =
        sysInfo.gpus[amdGpuIdx].sharedMemBytes / (1024.0f * 1024.0f);
    if (sharedMemMB > 0) {
      addOrUpdateRecord("Shared GPU Memory", gpuCat, sharedMemMB, "MB", "Data");
    }

    // GPU Engine utilization via PDH
    static PDH_HQUERY gpuQuery = nullptr;
    static PDH_HCOUNTER gpuCounter = nullptr;
    static bool pdhInitialized = false;
    static bool pdhAvailable = false;

    if (!pdhInitialized) {
      pdhInitialized = true;
      if (PdhOpenQueryW(nullptr, 0, &gpuQuery) == ERROR_SUCCESS) {
        PDH_STATUS status = PdhAddEnglishCounterW(
            gpuQuery, L"\\GPU Engine(*)\\Utilization Percentage", 0,
            &gpuCounter);
        if (status == ERROR_SUCCESS) {
          PdhCollectQueryData(gpuQuery);
          pdhAvailable = true;
        }
      }
    }

    if (pdhAvailable && gpuQuery) {
      PdhCollectQueryData(gpuQuery);
      PDH_FMT_COUNTERVALUE counterValue;
      if (PdhGetFormattedCounterValue(gpuCounter, PDH_FMT_DOUBLE, nullptr,
                                      &counterValue) == ERROR_SUCCESS) {
        float gpuUsage = (float)counterValue.doubleValue;
        if (gpuUsage >= 0 && gpuUsage <= 100) {
          addOrUpdateRecord("GPU Utilization", gpuCat, gpuUsage, "%", "Load");
        }
      }
    }
  }

  // === RAM Sensors ===
  {
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    if (GlobalMemoryStatusEx(&memStatus)) {
      float totalGB =
          (float)memStatus.ullTotalPhys / (1024.0f * 1024.0f * 1024.0f);
      float usedGB = (float)(memStatus.ullTotalPhys - memStatus.ullAvailPhys) /
                     (1024.0f * 1024.0f * 1024.0f);
      float usagePercent = (float)memStatus.dwMemoryLoad;

      addOrUpdateRecord("Physical Memory Used", "RAM", usedGB, "GB", "Data");
      addOrUpdateRecord("Physical Memory Total", "RAM", totalGB, "GB", "Data");
      addOrUpdateRecord("Physical Memory Load", "RAM", usagePercent, "%",
                        "Load");
      addOrUpdateRecord("Available Memory", "RAM",
                        (float)memStatus.ullAvailPhys /
                            (1024.0f * 1024.0f * 1024.0f),
                        "GB", "Data");

      float totalVirtGB =
          (float)memStatus.ullTotalVirtual / (1024.0f * 1024.0f * 1024.0f);
      float usedVirtGB =
          (float)(memStatus.ullTotalVirtual - memStatus.ullAvailVirtual) /
          (1024.0f * 1024.0f * 1024.0f);
      addOrUpdateRecord("Virtual Memory Used", "RAM", usedVirtGB, "GB", "Data");
      addOrUpdateRecord("Virtual Memory Total", "RAM", totalVirtGB, "GB",
                        "Data");

      float totalPageGB =
          (float)memStatus.ullTotalPageFile / (1024.0f * 1024.0f * 1024.0f);
      float usedPageGB =
          (float)(memStatus.ullTotalPageFile - memStatus.ullAvailPageFile) /
          (1024.0f * 1024.0f * 1024.0f);
      addOrUpdateRecord("Page File Used", "RAM", usedPageGB, "GB", "Data");
      addOrUpdateRecord("Page File Total", "RAM", totalPageGB, "GB", "Data");
    }

    if (hardwareDetected && sysInfo.ram.speedMHz > 0) {
      addOrUpdateRecord("Memory Clock", "RAM", (float)sysInfo.ram.speedMHz,
                        "MHz", "Clock");
    }
  }

  // Update summary tab live data
  updateSummaryTab();

  // Refresh the sensor table view
  populateSensorTable(currentCategory);
}

// ============================================================================
// Record Helpers
// ============================================================================
int SensorPanelWidget::findRecord(const QString &name,
                                  const QString &category) {
  for (int i = 0; i < sensorRecords.size(); ++i) {
    if (sensorRecords[i].name == name && sensorRecords[i].category == category)
      return i;
  }
  return -1;
}

void SensorPanelWidget::addOrUpdateRecord(const QString &name,
                                          const QString &category, float value,
                                          const QString &unit,
                                          const QString &sensorType) {
  int idx = findRecord(name, category);
  if (idx < 0) {
    SensorRecord rec;
    rec.name = name;
    rec.category = category;
    rec.current = value;
    rec.min = value;
    rec.max = value;
    rec.sum = value;
    rec.count = 1;
    rec.unit = unit;
    rec.sensorType = sensorType;
    sensorRecords.append(rec);
  } else {
    auto &rec = sensorRecords[idx];
    rec.current = value;
    if (value < rec.min)
      rec.min = value;
    if (value > rec.max)
      rec.max = value;
    rec.sum += value;
    rec.count++;
  }
}

// ============================================================================
// Color Helpers
// ============================================================================
QString SensorPanelWidget::tempColor(float temp) {
  if (temp >= 90)
    return "#FF4444"; // Critical red
  if (temp >= 75)
    return "#FF8C00"; // Warning orange
  if (temp >= 60)
    return "#DCDCAA"; // Warm yellow
  return "#6A9955";   // Cool green
}

QString SensorPanelWidget::loadColor(float load) {
  if (load >= 90)
    return "#FF6B6B"; // High red
  if (load >= 70)
    return "#CE9178"; // Medium orange
  if (load >= 40)
    return "#DCDCAA"; // Moderate yellow
  return "#6A9955";   // Low green
}
