/*
 * Sensor Panel Widget Implementation - HWiNFO64-style sensor monitoring
 * Left: Hardware tree  |  Right: Sensor data table with Current/Min/Max/Avg
 * License: MIT
 */

#include "sensor_panel_widget.h"
#include <QFont>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QPalette>
#include <QScrollArea>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Pdh.h>
#include <Windows.h>


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
  populateHardwareTree();

  // 2-second refresh for sensor readings
  updateTimer = new QTimer(this);
  connect(updateTimer, &QTimer::timeout, this, &SensorPanelWidget::updateData);
  updateTimer->start(2000);

  // Initial data
  updateData();
}

void SensorPanelWidget::setupUI() {
  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  // Top bar with title and status
  auto *topBar = new QWidget(this);
  topBar->setObjectName("sensorTopBar");
  topBar->setFixedHeight(32);
  auto *topLayout = new QHBoxLayout(topBar);
  topLayout->setContentsMargins(10, 4, 10, 4);

  titleLabel = new QLabel("Sensor Status", this);
  titleLabel->setObjectName("sensorTitle");

  statusLabel = new QLabel("Updating...", this);
  statusLabel->setObjectName("sensorStatus");

  topLayout->addWidget(titleLabel);
  topLayout->addStretch();
  topLayout->addWidget(statusLabel);
  mainLayout->addWidget(topBar);

  // Splitter: left tree | right table
  splitter = new QSplitter(Qt::Horizontal, this);

  // Left: Hardware tree
  hardwareTree = new QTreeWidget(this);
  hardwareTree->setHeaderLabel("Hardware");
  hardwareTree->setMinimumWidth(280);
  hardwareTree->setMaximumWidth(350);
  hardwareTree->setIndentation(16);
  hardwareTree->setAnimated(true);
  hardwareTree->setRootIsDecorated(true);
  connect(hardwareTree, &QTreeWidget::itemClicked, this,
          &SensorPanelWidget::onTreeItemClicked);

  // Right: Sensor data table
  sensorTable = new QTableWidget(this);
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
  sensorTable->verticalHeader()->setDefaultSectionSize(22);

  splitter->addWidget(hardwareTree);
  splitter->addWidget(sensorTable);
  splitter->setSizes({280, 900});

  mainLayout->addWidget(splitter);
}

void SensorPanelWidget::applyStyles() {
  setStyleSheet(R"(
    /* Top bar */
    #sensorTopBar {
        background-color: #2D2D30;
        border-bottom: 1px solid #3E3E42;
    }
    #sensorTitle {
        color: #CCCCCC;
        font-size: 13px;
        font-weight: bold;
        font-family: 'Segoe UI', sans-serif;
    }
    #sensorStatus {
        color: #6A9955;
        font-size: 11px;
        font-family: 'Segoe UI', sans-serif;
    }

    /* Hardware Tree (HWiNFO64 style - dark with colored items) */
    QTreeWidget {
        background-color: #1E1E1E;
        color: #CCCCCC;
        border: none;
        border-right: 1px solid #3E3E42;
        font-size: 12px;
        font-family: 'Segoe UI', sans-serif;
        outline: none;
    }
    QTreeWidget::item {
        padding: 3px 6px;
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
        padding: 4px 8px;
        font-size: 11px;
        font-weight: bold;
    }

    /* Sensor Table (HWiNFO64 data grid style) */
    QTableWidget {
        background-color: #1E1E1E;
        color: #CCCCCC;
        border: none;
        gridline-color: #2D2D30;
        font-size: 12px;
        font-family: 'Consolas', 'Courier New', monospace;
        outline: none;
        selection-background-color: #094771;
    }
    QTableWidget::item {
        padding: 2px 8px;
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
        width: 10px;
        border: none;
    }
    QScrollBar::handle:vertical {
        background-color: #424242;
        border-radius: 4px;
        min-height: 30px;
    }
    QScrollBar::handle:vertical:hover {
        background-color: #4F4F4F;
    }
    QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
        height: 0px;
    }
    QScrollBar:horizontal {
        background-color: #1E1E1E;
        height: 10px;
        border: none;
    }
    QScrollBar::handle:horizontal {
        background-color: #424242;
        border-radius: 4px;
        min-width: 30px;
    }

    /* Splitter */
    QSplitter::handle {
        background-color: #3E3E42;
        width: 1px;
    }
  )");
}

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
  auto *cpuTempItem = new QTreeWidgetItem(cpuItem);
  cpuTempItem->setText(0, "Temperatures");
  cpuTempItem->setData(0, Qt::UserRole, "CPU_Temp");
  cpuTempItem->setForeground(0, QColor("#CE9178"));

  auto *cpuClockItem = new QTreeWidgetItem(cpuItem);
  cpuClockItem->setText(0, "Clocks");
  cpuClockItem->setData(0, Qt::UserRole, "CPU_Clock");
  cpuClockItem->setForeground(0, QColor("#4EC9B0"));

  auto *cpuLoadItem = new QTreeWidgetItem(cpuItem);
  cpuLoadItem->setText(0, "Utilization");
  cpuLoadItem->setData(0, Qt::UserRole, "CPU_Load");
  cpuLoadItem->setForeground(0, QColor("#DCDCAA"));

  auto *cpuPowerItem = new QTreeWidgetItem(cpuItem);
  cpuPowerItem->setText(0, "Power");
  cpuPowerItem->setData(0, Qt::UserRole, "CPU_Power");
  cpuPowerItem->setForeground(0, QColor("#D7BA7D"));

  // GPU
  if (hardwareDetected && !sysInfo.gpus.empty()) {
    for (size_t i = 0; i < sysInfo.gpus.size(); ++i) {
      auto *gpuItem = new QTreeWidgetItem(systemItem);
      gpuItem->setText(0,
                       "GPU: " + QString::fromStdString(sysInfo.gpus[i].name));
      gpuItem->setData(0, Qt::UserRole, QString("GPU_%1").arg(i));
      gpuItem->setForeground(0, QColor("#CE9178"));
      gpuItem->setExpanded(true);

      auto *gpuTempItem = new QTreeWidgetItem(gpuItem);
      gpuTempItem->setText(0, "Temperatures");
      gpuTempItem->setData(0, Qt::UserRole, QString("GPU_%1_Temp").arg(i));
      gpuTempItem->setForeground(0, QColor("#CE9178"));

      auto *gpuClockItem = new QTreeWidgetItem(gpuItem);
      gpuClockItem->setText(0, "Clocks");
      gpuClockItem->setData(0, Qt::UserRole, QString("GPU_%1_Clock").arg(i));
      gpuClockItem->setForeground(0, QColor("#4EC9B0"));

      auto *gpuLoadItem = new QTreeWidgetItem(gpuItem);
      gpuLoadItem->setText(0, "Utilization");
      gpuLoadItem->setData(0, Qt::UserRole, QString("GPU_%1_Load").arg(i));
      gpuLoadItem->setForeground(0, QColor("#DCDCAA"));

      auto *gpuPowerItem = new QTreeWidgetItem(gpuItem);
      gpuPowerItem->setText(0, "Power");
      gpuPowerItem->setData(0, Qt::UserRole, QString("GPU_%1_Power").arg(i));
      gpuPowerItem->setForeground(0, QColor("#D7BA7D"));

      auto *gpuFanItem = new QTreeWidgetItem(gpuItem);
      gpuFanItem->setText(0, "Fans");
      gpuFanItem->setData(0, Qt::UserRole, QString("GPU_%1_Fan").arg(i));
      gpuFanItem->setForeground(0, QColor("#608B4E"));

      auto *gpuMemItem = new QTreeWidgetItem(gpuItem);
      gpuMemItem->setText(0, "Memory");
      gpuMemItem->setData(0, Qt::UserRole, QString("GPU_%1_Mem").arg(i));
      gpuMemItem->setForeground(0, QColor("#569CD6"));
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
    storageItem->setForeground(0, QColor("#808080"));

    for (size_t i = 0; i < sysInfo.storage.size(); ++i) {
      auto *driveItem = new QTreeWidgetItem(storageItem);
      driveItem->setText(0, QString::fromStdString(sysInfo.storage[i].name));
      driveItem->setData(0, Qt::UserRole, QString("Storage_%1").arg(i));
      driveItem->setForeground(0, QColor("#808080"));
    }
  }

  // Select root by default
  hardwareTree->setCurrentItem(systemItem);
}

void SensorPanelWidget::onTreeItemClicked(QTreeWidgetItem *item, int column) {
  Q_UNUSED(column);
  currentCategory = item->data(0, Qt::UserRole).toString();
  populateSensorTable(currentCategory);
}

void SensorPanelWidget::populateSensorTable(const QString &category) {
  sensorTable->setRowCount(0);

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
        // GPU-specific subcategory
        QString gpuPrefix = category.left(5); // "GPU_0" or "GPU_1"
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
          show = true; // Show all for "GPU_0"
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

    int row = sensorTable->rowCount();
    sensorTable->insertRow(row);

    // Sensor name
    auto *nameItem = new QTableWidgetItem(rec.name);
    nameItem->setForeground(QColor("#CCCCCC"));
    sensorTable->setItem(row, 0, nameItem);

    // Current value
    QString currentStr = QString("%1 %2")
                             .arg(rec.current, 0, 'f',
                                  rec.unit == "%" || rec.unit == "°C" ? 1 : 2)
                             .arg(rec.unit);
    auto *currentItem = new QTableWidgetItem(currentStr);
    // Color based on sensor type
    if (rec.sensorType == "Temperature") {
      currentItem->setForeground(QColor(tempColor(rec.current)));
    } else if (rec.sensorType == "Load") {
      currentItem->setForeground(QColor(loadColor(rec.current)));
    } else if (rec.sensorType == "Clock") {
      currentItem->setForeground(QColor("#4EC9B0"));
    } else if (rec.sensorType == "Power") {
      currentItem->setForeground(QColor("#D7BA7D"));
    } else if (rec.sensorType == "Fan") {
      currentItem->setForeground(QColor("#608B4E"));
    } else if (rec.sensorType == "Voltage") {
      currentItem->setForeground(QColor("#B5CEA8"));
    } else {
      currentItem->setForeground(QColor("#DCDCAA"));
    }
    currentItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    sensorTable->setItem(row, 1, currentItem);

    // Min
    float minVal = rec.min < 999999 ? rec.min : 0;
    auto *minItem = new QTableWidgetItem(
        QString("%1 %2")
            .arg(minVal, 0, 'f', rec.unit == "%" || rec.unit == "°C" ? 1 : 2)
            .arg(rec.unit));
    minItem->setForeground(QColor("#569CD6"));
    minItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    sensorTable->setItem(row, 2, minItem);

    // Max
    float maxVal = rec.max > -999999 ? rec.max : 0;
    auto *maxItem = new QTableWidgetItem(
        QString("%1 %2")
            .arg(maxVal, 0, 'f', rec.unit == "%" || rec.unit == "°C" ? 1 : 2)
            .arg(rec.unit));
    maxItem->setForeground(QColor("#CE9178"));
    maxItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    sensorTable->setItem(row, 3, maxItem);

    // Average
    float avgVal = rec.count > 0 ? rec.sum / rec.count : 0;
    auto *avgItem = new QTableWidgetItem(
        QString("%1 %2")
            .arg(avgVal, 0, 'f', rec.unit == "%" || rec.unit == "°C" ? 1 : 2)
            .arg(rec.unit));
    avgItem->setForeground(QColor("#808080"));
    avgItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    sensorTable->setItem(row, 4, avgItem);
  }

  statusLabel->setText(
      QString("%1 sensors | Updated").arg(sensorTable->rowCount()));
}

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
  // GPU index mapping: find NVIDIA and AMD GPUs
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

  // AMD iGPU - show hardware-detected data and GPU engine utilization
  if (amdGpuIdx >= 0 && hardwareDetected &&
      amdGpuIdx < (int)sysInfo.gpus.size()) {
    QString gpuCat = QString("GPU_%1").arg(amdGpuIdx);

    // Show detected VRAM info from WMI
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

    // GPU Engine utilization via PDH (Performance Data Helper)
    // Query "\GPU Engine(*engtype_3D)\Utilization Percentage" for 3D engine
    static PDH_HQUERY gpuQuery = nullptr;
    static PDH_HCOUNTER gpuCounter = nullptr;
    static bool pdhInitialized = false;
    static bool pdhAvailable = false;

    if (!pdhInitialized) {
      pdhInitialized = true;
      if (PdhOpenQueryW(nullptr, 0, &gpuQuery) == ERROR_SUCCESS) {
        // Try AMD-specific GPU engine counter
        // The counter path uses the adapter LUID which varies, so use wildcard
        PDH_STATUS status = PdhAddEnglishCounterW(
            gpuQuery, L"\\GPU Engine(*)\\Utilization Percentage", 0,
            &gpuCounter);
        if (status == ERROR_SUCCESS) {
          PdhCollectQueryData(gpuQuery); // First collection to prime
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

  // === RAM Sensors (via GlobalMemoryStatusEx - reliable) ===
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

      // Virtual memory
      float totalVirtGB =
          (float)memStatus.ullTotalVirtual / (1024.0f * 1024.0f * 1024.0f);
      float usedVirtGB =
          (float)(memStatus.ullTotalVirtual - memStatus.ullAvailVirtual) /
          (1024.0f * 1024.0f * 1024.0f);
      addOrUpdateRecord("Virtual Memory Used", "RAM", usedVirtGB, "GB", "Data");
      addOrUpdateRecord("Virtual Memory Total", "RAM", totalVirtGB, "GB",
                        "Data");

      // Page file
      float totalPageGB =
          (float)memStatus.ullTotalPageFile / (1024.0f * 1024.0f * 1024.0f);
      float usedPageGB =
          (float)(memStatus.ullTotalPageFile - memStatus.ullAvailPageFile) /
          (1024.0f * 1024.0f * 1024.0f);
      addOrUpdateRecord("Page File Used", "RAM", usedPageGB, "GB", "Data");
      addOrUpdateRecord("Page File Total", "RAM", totalPageGB, "GB", "Data");
    }

    // RAM speed from hardware detector
    if (hardwareDetected && sysInfo.ram.speedMHz > 0) {
      addOrUpdateRecord("Memory Clock", "RAM", (float)sysInfo.ram.speedMHz,
                        "MHz", "Clock");
    }
  }

  // Refresh the table view
  populateSensorTable(currentCategory);
}

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
