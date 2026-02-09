/*
 * Dashboard Widget Implementation
 * Professional UI combining HWiNFO64, 3DMark, and Geekbench styles
 * License: MIT
 */

#include "dashboard_widget.h"
#include <QFont>
#include <QGroupBox>
#include <QPalette>
#include <QScrollArea>

#include <Windows.h>

DashboardWidget::DashboardWidget(QWidget *parent) : QWidget(parent) {
  // Initialize hardware detector
  hwDetector = std::make_unique<HardwareDetector>();
  if (hwDetector->Initialize()) {
    sysInfo = hwDetector->DetectAll();
    hardwareDetected = true;
  }

  // Initialize sensor monitor (for real-time GPU sensors via NVML)
  sensorMonitor = std::make_unique<SensorMonitor>();
  nvmlAvailable = sensorMonitor->InitNVML();

  setupUI();
  applyStyles();

  // Setup update timer (2 second refresh for sensor readings)
  updateTimer = new QTimer(this);
  connect(updateTimer, &QTimer::timeout, this, &DashboardWidget::updateData);
  updateTimer->start(2000);

  // Initial data load
  updateData();
}

void DashboardWidget::setupUI() {
  mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(20, 20, 20, 20);
  mainLayout->setSpacing(15);

  // Title
  QLabel *titleLabel = new QLabel("System Overview", this);
  titleLabel->setObjectName("sectionTitle");
  mainLayout->addWidget(titleLabel);

  // System Summary Section (3DMark style horizontal cards)
  createSystemSummarySection();

  // Sensors Section Title
  QLabel *sensorsTitle = new QLabel("Hardware Sensors", this);
  sensorsTitle->setObjectName("sectionTitle");
  mainLayout->addWidget(sensorsTitle);

  // Sensors Grid (HWiNFO64 style) - horizontal layout of sensor cards
  QHBoxLayout *sensorsLayout = new QHBoxLayout();
  sensorsLayout->setSpacing(15);

  // Create CPU Sensor Card
  QGroupBox *cpuGroup = new QGroupBox("CPU Sensors", this);
  cpuGroup->setObjectName("sensorGroup");
  QGridLayout *cpuGrid = new QGridLayout(cpuGroup);
  cpuGrid->setSpacing(8);

  int row = 0;
  cpuGrid->addWidget(new QLabel("Cores / Threads:", this), row, 0);
  lblCPUCores = new QLabel("-- / --", this);
  lblCPUCores->setObjectName("sensorValue");
  cpuGrid->addWidget(lblCPUCores, row++, 1);

  cpuGrid->addWidget(new QLabel("Temperature:", this), row, 0);
  lblCPUTemp = new QLabel("-- °C", this);
  lblCPUTemp->setObjectName("sensorValueTemp");
  cpuGrid->addWidget(lblCPUTemp, row++, 1);

  cpuGrid->addWidget(new QLabel("Frequency:", this), row, 0);
  lblCPUFreq = new QLabel("-- MHz", this);
  lblCPUFreq->setObjectName("sensorValueFreq");
  cpuGrid->addWidget(lblCPUFreq, row++, 1);

  cpuGrid->addWidget(new QLabel("Power:", this), row, 0);
  lblCPUPower = new QLabel("-- W", this);
  lblCPUPower->setObjectName("sensorValue");
  cpuGrid->addWidget(lblCPUPower, row++, 1);

  cpuGrid->addWidget(new QLabel("Usage:", this), row, 0);
  QHBoxLayout *cpuUsageLayout = new QHBoxLayout();
  cpuUsageBar = new QProgressBar(this);
  cpuUsageBar->setObjectName("cpuBar");
  cpuUsageBar->setRange(0, 100);
  cpuUsageBar->setValue(0);
  cpuUsageBar->setTextVisible(false);
  cpuUsageBar->setFixedHeight(12);
  lblCPUUsage = new QLabel("-- %", this);
  lblCPUUsage->setObjectName("sensorValue");
  cpuUsageLayout->addWidget(cpuUsageBar);
  cpuUsageLayout->addWidget(lblCPUUsage);
  cpuGrid->addLayout(cpuUsageLayout, row++, 1);

  sensorsLayout->addWidget(cpuGroup);

  // Create GPU Sensor Card
  QGroupBox *gpuGroup = new QGroupBox("GPU Sensors", this);
  gpuGroup->setObjectName("sensorGroup");
  QGridLayout *gpuGrid = new QGridLayout(gpuGroup);
  gpuGrid->setSpacing(8);

  row = 0;
  gpuGrid->addWidget(new QLabel("Temperature:", this), row, 0);
  lblGPUTemp = new QLabel("-- °C", this);
  lblGPUTemp->setObjectName("sensorValueTemp");
  gpuGrid->addWidget(lblGPUTemp, row++, 1);

  gpuGrid->addWidget(new QLabel("GPU Clock:", this), row, 0);
  lblGPUClock = new QLabel("-- MHz", this);
  lblGPUClock->setObjectName("sensorValueFreq");
  gpuGrid->addWidget(lblGPUClock, row++, 1);

  gpuGrid->addWidget(new QLabel("Power:", this), row, 0);
  lblGPUPower = new QLabel("-- W", this);
  lblGPUPower->setObjectName("sensorValue");
  gpuGrid->addWidget(lblGPUPower, row++, 1);

  gpuGrid->addWidget(new QLabel("Fan Speed:", this), row, 0);
  lblGPUFanSpeed = new QLabel("-- RPM", this);
  lblGPUFanSpeed->setObjectName("sensorValue");
  gpuGrid->addWidget(lblGPUFanSpeed, row++, 1);

  gpuGrid->addWidget(new QLabel("GPU Usage:", this), row, 0);
  QHBoxLayout *gpuUsageLayout = new QHBoxLayout();
  gpuUsageBar = new QProgressBar(this);
  gpuUsageBar->setObjectName("gpuBar");
  gpuUsageBar->setRange(0, 100);
  gpuUsageBar->setValue(0);
  gpuUsageBar->setTextVisible(false);
  gpuUsageBar->setFixedHeight(12);
  lblGPUUsage = new QLabel("-- %", this);
  lblGPUUsage->setObjectName("sensorValue");
  gpuUsageLayout->addWidget(gpuUsageBar);
  gpuUsageLayout->addWidget(lblGPUUsage);
  gpuGrid->addLayout(gpuUsageLayout, row++, 1);

  gpuGrid->addWidget(new QLabel("VRAM:", this), row, 0);
  QHBoxLayout *vramLayout = new QHBoxLayout();
  vramUsageBar = new QProgressBar(this);
  vramUsageBar->setObjectName("vramBar");
  vramUsageBar->setRange(0, 100);
  vramUsageBar->setValue(0);
  vramUsageBar->setTextVisible(false);
  vramUsageBar->setFixedHeight(12);
  lblGPUVRAM = new QLabel("-- / -- GB", this);
  lblGPUVRAM->setObjectName("sensorValue");
  vramLayout->addWidget(vramUsageBar);
  vramLayout->addWidget(lblGPUVRAM);
  gpuGrid->addLayout(vramLayout, row++, 1);

  sensorsLayout->addWidget(gpuGroup);

  // Create RAM Sensor Card
  QGroupBox *ramGroup = new QGroupBox("Memory", this);
  ramGroup->setObjectName("sensorGroup");
  QGridLayout *ramGrid = new QGridLayout(ramGroup);
  ramGrid->setSpacing(8);

  row = 0;
  ramGrid->addWidget(new QLabel("Total:", this), row, 0);
  lblRAMTotal = new QLabel("-- GB", this);
  lblRAMTotal->setObjectName("sensorValue");
  ramGrid->addWidget(lblRAMTotal, row++, 1);

  ramGrid->addWidget(new QLabel("Speed:", this), row, 0);
  lblRAMSpeed = new QLabel("-- MHz", this);
  lblRAMSpeed->setObjectName("sensorValueFreq");
  ramGrid->addWidget(lblRAMSpeed, row++, 1);

  ramGrid->addWidget(new QLabel("Used:", this), row, 0);
  QHBoxLayout *ramUsageLayout = new QHBoxLayout();
  ramUsageBar = new QProgressBar(this);
  ramUsageBar->setObjectName("ramBar");
  ramUsageBar->setRange(0, 100);
  ramUsageBar->setValue(0);
  ramUsageBar->setTextVisible(false);
  ramUsageBar->setFixedHeight(12);
  lblRAMUsed = new QLabel("-- / -- GB", this);
  lblRAMUsed->setObjectName("sensorValue");
  ramUsageLayout->addWidget(ramUsageBar);
  ramUsageLayout->addWidget(lblRAMUsed);
  ramGrid->addLayout(ramUsageLayout, row++, 1);

  sensorsLayout->addWidget(ramGroup);

  mainLayout->addLayout(sensorsLayout);

  // Benchmark Cards Section (3DMark style)
  createBenchmarkCardsSection();

  mainLayout->addStretch();
}

void DashboardWidget::createSystemSummarySection() {
  QFrame *summaryFrame = new QFrame(this);
  summaryFrame->setObjectName("systemSummary");
  summaryFrame->setFrameShape(QFrame::StyledPanel);

  QHBoxLayout *layout = new QHBoxLayout(summaryFrame);
  layout->setSpacing(30);

  // GPU Card (3DMark style)
  QVBoxLayout *gpuCard = new QVBoxLayout();
  QLabel *gpuIcon = new QLabel("GPU", this);
  gpuIcon->setObjectName("cardIcon");
  lblGPUModel = new QLabel("Detecting...", this);
  lblGPUModel->setObjectName("cardValue");
  gpuCard->addWidget(gpuIcon, 0, Qt::AlignCenter);
  gpuCard->addWidget(lblGPUModel, 0, Qt::AlignCenter);
  layout->addLayout(gpuCard);

  // CPU Card
  QVBoxLayout *cpuCard = new QVBoxLayout();
  QLabel *cpuIcon = new QLabel("CPU", this);
  cpuIcon->setObjectName("cardIcon");
  lblCPUModel = new QLabel("Detecting...", this);
  lblCPUModel->setObjectName("cardValue");
  cpuCard->addWidget(cpuIcon, 0, Qt::AlignCenter);
  cpuCard->addWidget(lblCPUModel, 0, Qt::AlignCenter);
  layout->addLayout(cpuCard);

  // RAM Card
  QVBoxLayout *ramCard = new QVBoxLayout();
  QLabel *ramIcon = new QLabel("Memory", this);
  ramIcon->setObjectName("cardIcon");
  lblRAMInfo = new QLabel("Detecting...", this);
  lblRAMInfo->setObjectName("cardValue");
  ramCard->addWidget(ramIcon, 0, Qt::AlignCenter);
  ramCard->addWidget(lblRAMInfo, 0, Qt::AlignCenter);
  layout->addLayout(ramCard);

  // Storage Card
  QVBoxLayout *storageCard = new QVBoxLayout();
  QLabel *storageIcon = new QLabel("Storage", this);
  storageIcon->setObjectName("cardIcon");
  lblStorageInfo = new QLabel("Detecting...", this);
  lblStorageInfo->setObjectName("cardValue");
  storageCard->addWidget(storageIcon, 0, Qt::AlignCenter);
  storageCard->addWidget(lblStorageInfo, 0, Qt::AlignCenter);
  layout->addLayout(storageCard);

  mainLayout->addWidget(summaryFrame);
}

void DashboardWidget::createCPUSection() {
  // Not used - inline in setupUI
}

void DashboardWidget::createGPUSection() {
  // Not used - inline in setupUI
}

void DashboardWidget::createRAMSection() {
  // Not used - inline in setupUI
}

void DashboardWidget::createBenchmarkCardsSection() {
  QLabel *benchTitle = new QLabel("Quick Benchmarks", this);
  benchTitle->setObjectName("sectionTitle");
  mainLayout->addWidget(benchTitle);

  QHBoxLayout *cardsLayout = new QHBoxLayout();
  cardsLayout->setSpacing(15);

  // CPU Benchmark Card
  QFrame *cpuBenchCard = new QFrame(this);
  cpuBenchCard->setObjectName("benchmarkCard");
  QVBoxLayout *cpuBenchLayout = new QVBoxLayout(cpuBenchCard);
  QLabel *cpuBenchTitle = new QLabel("CPU Benchmark", this);
  cpuBenchTitle->setObjectName("benchCardTitle");
  QLabel *cpuBenchDesc =
      new QLabel("Single & Multi-core\nperformance test", this);
  cpuBenchDesc->setObjectName("benchCardDesc");
  cpuBenchLayout->addWidget(cpuBenchTitle);
  cpuBenchLayout->addWidget(cpuBenchDesc);
  cpuBenchLayout->addStretch();
  cardsLayout->addWidget(cpuBenchCard);

  // GPU Benchmark Card
  QFrame *gpuBenchCard = new QFrame(this);
  gpuBenchCard->setObjectName("benchmarkCard");
  QVBoxLayout *gpuBenchLayout = new QVBoxLayout(gpuBenchCard);
  QLabel *gpuBenchTitle = new QLabel("GPU Benchmark", this);
  gpuBenchTitle->setObjectName("benchCardTitle");
  QLabel *gpuBenchDesc = new QLabel("DirectX 12 & Vulkan\ngraphics test", this);
  gpuBenchDesc->setObjectName("benchCardDesc");
  gpuBenchLayout->addWidget(gpuBenchTitle);
  gpuBenchLayout->addWidget(gpuBenchDesc);
  gpuBenchLayout->addStretch();
  cardsLayout->addWidget(gpuBenchCard);

  // AI Benchmark Card
  QFrame *aiBenchCard = new QFrame(this);
  aiBenchCard->setObjectName("benchmarkCard");
  QVBoxLayout *aiBenchLayout = new QVBoxLayout(aiBenchCard);
  QLabel *aiBenchTitle = new QLabel("AI Benchmark", this);
  aiBenchTitle->setObjectName("benchCardTitle");
  QLabel *aiBenchDesc = new QLabel("ONNX & OpenVINO\ninference test", this);
  aiBenchDesc->setObjectName("benchCardDesc");
  aiBenchLayout->addWidget(aiBenchTitle);
  aiBenchLayout->addWidget(aiBenchDesc);
  aiBenchLayout->addStretch();
  cardsLayout->addWidget(aiBenchCard);

  mainLayout->addLayout(cardsLayout);
}

void DashboardWidget::applyStyles() {
  setStyleSheet(R"(
        /* Section Titles */
        #sectionTitle {
            font-size: 18px;
            font-weight: bold;
            color: #FFFFFF;
            margin-bottom: 10px;
        }
        
        /* System Summary Frame (3DMark style) */
        #systemSummary {
            background-color: #2D2D30;
            border: 1px solid #3E3E42;
            border-radius: 8px;
            padding: 15px;
        }
        
        #cardIcon {
            font-size: 14px;
            font-weight: bold;
            color: #808080;
        }
        
        #cardValue {
            font-size: 11px;
            color: #CCCCCC;
        }
        
        /* Sensor Groups (HWiNFO64 style) */
        QGroupBox#sensorGroup {
            background-color: #252526;
            border: 1px solid #3E3E42;
            border-radius: 6px;
            font-weight: bold;
            color: #569CD6;
            padding: 10px;
            margin-top: 10px;
        }
        
        QGroupBox#sensorGroup::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px;
        }
        
        #sensorValue {
            color: #DCDCAA;
            font-family: 'Consolas', 'Courier New', monospace;
            font-size: 12px;
        }
        
        #sensorValueTemp {
            color: #FF6B6B;
            font-family: 'Consolas', monospace;
            font-size: 12px;
            font-weight: bold;
        }
        
        #sensorValueFreq {
            color: #4EC9B0;
            font-family: 'Consolas', monospace;
            font-size: 12px;
        }
        
        /* Progress Bars (Color-coded like HWiNFO64) */
        QProgressBar {
            background-color: #3E3E42;
            border: none;
            border-radius: 3px;
        }
        
        #cpuBar::chunk {
            background-color: #569CD6;
            border-radius: 3px;
        }
        
        #gpuBar::chunk {
            background-color: #CE9178;
            border-radius: 3px;
        }
        
        #vramBar::chunk {
            background-color: #DCDCAA;
            border-radius: 3px;
        }
        
        #ramBar::chunk {
            background-color: #6A9955;
            border-radius: 3px;
        }
        
        /* Benchmark Cards (3DMark style) */
        #benchmarkCard {
            background-color: #2D2D30;
            border: 1px solid #3E3E42;
            border-radius: 8px;
            padding: 15px;
            min-height: 80px;
        }
        
        #benchmarkCard:hover {
            border-color: #007ACC;
            background-color: #37373D;
        }
        
        #benchCardTitle {
            font-size: 14px;
            font-weight: bold;
            color: #FFFFFF;
        }
        
        #benchCardDesc {
            font-size: 11px;
            color: #808080;
        }
    )");
}

void DashboardWidget::updateData() {
  // Use real hardware detection data
  if (hardwareDetected) {
    // Update RAM info (this changes dynamically)
    sysInfo.ram = hwDetector->DetectRAM();
  }

  // System Info (from WMI detection)
  if (hardwareDetected && !sysInfo.cpu.name.empty()) {
    lblCPUModel->setText(QString::fromStdString(sysInfo.cpu.name));
  } else {
    lblCPUModel->setText("Detection failed");
  }

  if (hardwareDetected && !sysInfo.gpus.empty()) {
    lblGPUModel->setText(QString::fromStdString(sysInfo.gpus[0].name));
  } else {
    lblGPUModel->setText("No GPU detected");
  }

  if (hardwareDetected) {
    float totalGB = sysInfo.ram.totalBytes / (1024.0f * 1024.0f * 1024.0f);
    QString ramType = QString::fromStdString(sysInfo.ram.type);
    int speed = sysInfo.ram.speedMHz;
    lblRAMInfo->setText(
        QString("%1 GB %2-%3").arg((int)totalGB).arg(ramType).arg(speed));
  } else {
    lblRAMInfo->setText("-- GB");
  }

  if (hardwareDetected && !sysInfo.storage.empty()) {
    float sizeGB = sysInfo.storage[0].sizeBytes / (1024.0f * 1024.0f * 1024.0f);
    QString type = QString::fromStdString(sysInfo.storage[0].interfaceType);
    lblStorageInfo->setText(
        QString("%1 GB %2 %3")
            .arg((int)sizeGB)
            .arg(type)
            .arg(QString::fromStdString(sysInfo.storage[0].type)));
  } else {
    lblStorageInfo->setText("-- GB SSD");
  }

  // CPU Sensors
  if (hardwareDetected) {
    lblCPUCores->setText(QString("%1 / %2")
                             .arg(sysInfo.cpu.physicalCores)
                             .arg(sysInfo.cpu.logicalCores));
    lblCPUFreq->setText(QString("%1 MHz").arg(sysInfo.cpu.maxClockMHz));
  } else {
    lblCPUCores->setText("-- / --");
    lblCPUFreq->setText("-- MHz");
  }

  // Dynamic sensor data from SensorMonitor (WMI thermal zones + power
  // estimation)
  if (sensorMonitor) {
    auto cpuSensors = sensorMonitor->ReadCPUSensorsWMI();

    if (cpuSensors.packageTemp > 0) {
      lblCPUTemp->setText(QString("%1 °C").arg((int)cpuSensors.packageTemp));
    } else {
      lblCPUTemp->setText("-- °C");
    }

    if (cpuSensors.packagePowerW > 0) {
      lblCPUPower->setText(
          QString("%1 W").arg(cpuSensors.packagePowerW, 0, 'f', 1));
    } else {
      lblCPUPower->setText("-- W");
    }
  } else {
    lblCPUTemp->setText("-- °C");
    lblCPUPower->setText("-- W");
  }

  // Get actual CPU usage using Windows API
  FILETIME idleTime, kernelTime, userTime;
  static uint64_t prevIdle = 0, prevKernel = 0, prevUser = 0;
  int cpuUsage = 0;

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
        cpuUsage = (int)(100.0 * (1.0 - (double)idleDiff / (double)totalDiff));
      }
    }

    prevIdle = idle;
    prevKernel = kernel;
    prevUser = user;
  }
  lblCPUUsage->setText(QString("%1 %").arg(cpuUsage));
  cpuUsageBar->setValue(cpuUsage);

  // GPU Sensors - Real-time via NVML
  if (nvmlAvailable && sensorMonitor) {
    auto gpuSensors = sensorMonitor->ReadNVIDIASensors();

    if (gpuSensors.temperature > 0) {
      lblGPUTemp->setText(QString("%1 °C").arg((int)gpuSensors.temperature));
    } else {
      lblGPUTemp->setText("-- °C");
    }

    if (gpuSensors.gpuClock > 0) {
      lblGPUClock->setText(QString("%1 MHz").arg((int)gpuSensors.gpuClock));
    } else {
      lblGPUClock->setText("-- MHz");
    }

    if (gpuSensors.powerW > 0) {
      lblGPUPower->setText(QString("%1 W").arg(gpuSensors.powerW, 0, 'f', 1));
    } else {
      lblGPUPower->setText("-- W");
    }

    if (gpuSensors.fanSpeedPercent > 0) {
      lblGPUFanSpeed->setText(QString("%1 % / ~%2 RPM")
                                  .arg((int)gpuSensors.fanSpeedPercent)
                                  .arg((int)gpuSensors.fanSpeedRPM));
    } else {
      lblGPUFanSpeed->setText("-- RPM");
    }

    lblGPUUsage->setText(QString("%1 %").arg((int)gpuSensors.usagePercent));
    gpuUsageBar->setValue((int)gpuSensors.usagePercent);

    if (gpuSensors.memoryTotalMB > 0) {
      float usedGB = gpuSensors.memoryUsedMB / 1024.0f;
      float totalGB = gpuSensors.memoryTotalMB / 1024.0f;
      int vramPercent = (int)((usedGB / totalGB) * 100.0f);
      lblGPUVRAM->setText(
          QString("%1 / %2 GB").arg(usedGB, 0, 'f', 1).arg((int)totalGB));
      vramUsageBar->setValue(vramPercent);
    } else if (hardwareDetected && !sysInfo.gpus.empty()) {
      float vramGB = sysInfo.gpus[0].vramBytes / (1024.0f * 1024.0f * 1024.0f);
      lblGPUVRAM->setText(QString("-- / %1 GB").arg((int)vramGB));
      vramUsageBar->setValue(0);
    } else {
      lblGPUVRAM->setText("-- / -- GB");
      vramUsageBar->setValue(0);
    }
  } else {
    // Fallback - no NVML available
    lblGPUTemp->setText("-- °C");
    lblGPUClock->setText("-- MHz");
    lblGPUPower->setText("-- W");
    lblGPUFanSpeed->setText("-- RPM");
    lblGPUUsage->setText("-- %");
    gpuUsageBar->setValue(0);

    if (hardwareDetected && !sysInfo.gpus.empty()) {
      float vramGB = sysInfo.gpus[0].vramBytes / (1024.0f * 1024.0f * 1024.0f);
      lblGPUVRAM->setText(QString("-- / %1 GB").arg((int)vramGB));
    } else {
      lblGPUVRAM->setText("-- / -- GB");
    }
    vramUsageBar->setValue(0);
  }

  // RAM Sensors (real data from GlobalMemoryStatusEx)
  if (hardwareDetected) {
    float totalGB = sysInfo.ram.totalBytes / (1024.0f * 1024.0f * 1024.0f);
    float usedGB = (sysInfo.ram.totalBytes - sysInfo.ram.availableBytes) /
                   (1024.0f * 1024.0f * 1024.0f);
    int usagePercent = (int)((usedGB / totalGB) * 100.0f);

    lblRAMTotal->setText(QString("%1 GB").arg((int)totalGB));
    lblRAMSpeed->setText(QString("%1 MHz").arg(sysInfo.ram.speedMHz));
    lblRAMUsed->setText(
        QString("%1 / %2 GB").arg(usedGB, 0, 'f', 1).arg((int)totalGB));
    ramUsageBar->setValue(usagePercent);
  } else {
    lblRAMTotal->setText("-- GB");
    lblRAMSpeed->setText("-- MHz");
    lblRAMUsed->setText("-- / -- GB");
    ramUsageBar->setValue(0);
  }
}
