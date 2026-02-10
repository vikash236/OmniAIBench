/*
 * Sensor Panel Widget Implementation - HWiNFO64-style sensor monitoring
 * Tab 1: System Summary dashboard with vendor logos, mini charts, hw panels
 * Tab 2: Sensor Status with hardware tree + data table (Current/Min/Max/Avg)
 * License: MIT
 */

#include "sensor_panel_widget.h"
#include <QChartView>
#include <QFont>
#include <QHBoxLayout>
#include <QLineSeries>
#include <QPalette>
#include <QValueAxis>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Pdh.h>
#include <Windows.h>

// ============================================================================
// Constructor
// ============================================================================
SensorPanelWidget::SensorPanelWidget(QWidget *parent) : QWidget(parent) {
  hwDetector = std::make_unique<HardwareDetector>();
  if (hwDetector->Initialize()) {
    sysInfo = hwDetector->DetectAll();
    hardwareDetected = true;
  }
  sensorMonitor = std::make_unique<SensorMonitor>();
  nvmlAvailable = sensorMonitor->InitNVML();

  setupUI();
  applyStyles();

  updateTimer = new QTimer(this);
  connect(updateTimer, &QTimer::timeout, this, &SensorPanelWidget::updateData);
  updateTimer->start(2000);
  updateData();
}

// ============================================================================
// Helpers
// ============================================================================
QGroupBox *SensorPanelWidget::createInfoGroup(const QString &title,
                                              const QColor &borderColor) {
  auto *group = new QGroupBox(title);
  group->setObjectName("infoGroup");
  group->setStyleSheet(
      QString("QGroupBox#infoGroup { border: 1px solid %1; border-radius: 3px; "
              "margin-top: 12px; padding: 8px 6px 6px 6px; font-size: 12px; "
              "font-weight: bold; color: %1; background-color: #1A1D21; } "
              "QGroupBox#infoGroup::title { subcontrol-origin: margin; "
              "left: 8px; padding: 0 4px; color: %1; }")
          .arg(borderColor.name()));
  return group;
}

QLabel *SensorPanelWidget::createKeyLabel(const QString &text) {
  auto *l = new QLabel(text);
  l->setStyleSheet("color: #808080; font-size: 11px; font-family: 'Segoe UI';");
  return l;
}

QLabel *SensorPanelWidget::createValueLabel(const QString &text) {
  auto *l = new QLabel(text);
  l->setStyleSheet("color: #CCCCCC; font-size: 11px; font-family: 'Consolas';");
  l->setTextInteractionFlags(Qt::TextSelectableByMouse);
  return l;
}

QLabel *SensorPanelWidget::createVendorLogo(const QString &vendor) {
  auto *logo = new QLabel();
  logo->setFixedSize(80, 36);
  logo->setAlignment(Qt::AlignCenter);
  if (vendor == "AMD" || vendor.contains("Radeon") ||
      vendor.contains("Ryzen")) {
    logo->setText("AMD");
    logo->setStyleSheet("background: qlineargradient(x1:0,y1:0,x2:1,y2:1, "
                        "stop:0 #ED1C24, stop:1 #FF4444);"
                        "color: white; font-size: 16px; font-weight: 900; "
                        "font-family: 'Arial Black';"
                        "border-radius: 4px; padding: 2px 8px;");
  } else if (vendor == "NVIDIA" || vendor.contains("GeForce")) {
    logo->setText("NVIDIA");
    logo->setStyleSheet("background: qlineargradient(x1:0,y1:0,x2:1,y2:1, "
                        "stop:0 #76B900, stop:1 #4CAF50);"
                        "color: white; font-size: 14px; font-weight: 900; "
                        "font-family: 'Arial Black';"
                        "border-radius: 4px; padding: 2px 6px;");
  } else if (vendor == "Intel" || vendor.contains("Intel")) {
    logo->setText("intel");
    logo->setStyleSheet(
        "background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #0071C5, "
        "stop:1 #00A1E4);"
        "color: white; font-size: 15px; font-weight: 700; font-family: 'Arial';"
        "border-radius: 4px; padding: 2px 8px;");
  } else {
    logo->setText("GPU");
    logo->setStyleSheet(
        "background: #333; color: #CCC; font-size: 14px; font-weight: bold; "
        "border-radius: 4px; padding: 2px 8px;");
  }
  return logo;
}

QChartView *SensorPanelWidget::createMiniChart(const QString &title,
                                               const QColor &lineColor) {
  auto *series = new QLineSeries();
  series->setPen(QPen(lineColor, 1.5));
  auto *chart = new QChart();
  chart->addSeries(series);
  chart->setTitle(title);
  chart->setTitleFont(QFont("Segoe UI", 8));
  chart->setTitleBrush(QBrush(QColor("#808080")));
  chart->setBackgroundBrush(QBrush(QColor("#141414")));
  chart->setBackgroundRoundness(4);
  chart->legend()->hide();
  chart->setMargins(QMargins(2, 2, 2, 2));

  auto *axisX = new QValueAxis();
  axisX->setRange(0, 60);
  axisX->setVisible(false);
  chart->addAxis(axisX, Qt::AlignBottom);
  series->attachAxis(axisX);

  auto *axisY = new QValueAxis();
  axisY->setRange(0, 100);
  axisY->setLabelFormat("%d");
  axisY->setLabelsColor(QColor("#555"));
  axisY->setLabelsFont(QFont("Consolas", 7));
  axisY->setGridLineColor(QColor("#2A2A2A"));
  axisY->setTickCount(3);
  chart->addAxis(axisY, Qt::AlignLeft);
  series->attachAxis(axisY);

  auto *view = new QChartView(chart);
  view->setRenderHint(QPainter::Antialiasing);
  view->setFixedHeight(90);
  view->setStyleSheet("background: transparent; border: none;");
  return view;
}

void SensorPanelWidget::updateMiniChart(QChartView *chartView,
                                        QLineSeries *series, float value) {
  if (!chartView || !series)
    return;
  series->append(chartSampleCount, value);
  if (series->count() > 60)
    series->remove(0);
  auto *axisX = qobject_cast<QValueAxis *>(
      chartView->chart()->axes(Qt::Horizontal).first());
  if (axisX) {
    int minX = chartSampleCount > 60 ? chartSampleCount - 60 : 0;
    axisX->setRange(minX, chartSampleCount);
  }
}

// ============================================================================
// UI Setup
// ============================================================================
void SensorPanelWidget::setupUI() {
  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  tabWidget = new QTabWidget(this);
  tabWidget->setObjectName("sensorTabs");

  summaryTab = new QWidget();
  setupSummaryTab(summaryTab);
  tabWidget->addTab(summaryTab, "  System Summary  ");

  auto *sensorTab = new QWidget();
  setupSensorTab(sensorTab);
  tabWidget->addTab(sensorTab, "  Sensor Status  ");

  mainLayout->addWidget(tabWidget);
}

// ============================================================================
// Summary Tab
// ============================================================================
void SensorPanelWidget::setupSummaryTab(QWidget *tab) {
  auto *scrollArea = new QScrollArea(tab);
  scrollArea->setWidgetResizable(true);
  scrollArea->setFrameShape(QFrame::NoFrame);
  scrollArea->setStyleSheet("background-color: #1E1E1E; border: none;");

  auto *content = new QWidget();
  content->setStyleSheet("background-color: #1E1E1E;");
  auto *grid = new QGridLayout(content);
  grid->setContentsMargins(8, 4, 8, 8);
  grid->setSpacing(6);

  // --- CPU Panel ---
  QString cpuVendor =
      hardwareDetected ? QString::fromStdString(sysInfo.cpu.vendor) : "";
  auto *cpuGroup = createInfoGroup("CPU", QColor("#569CD6"));
  auto *cpuLay = new QGridLayout(cpuGroup);
  cpuLay->setSpacing(3);
  cpuLay->setContentsMargins(6, 4, 6, 4);

  // Vendor logo
  QString cpuLogoVendor = cpuVendor.contains("AMD")
                              ? "AMD"
                              : (cpuVendor.contains("Intel") ? "Intel" : "CPU");
  cpuLay->addWidget(createVendorLogo(cpuLogoVendor), 0, 0, 2, 1);

  cpuLay->addWidget(createKeyLabel("Processor"), 0, 1);
  cpuNameLabel = createValueLabel(
      hardwareDetected ? QString::fromStdString(sysInfo.cpu.name) : "-");
  cpuNameLabel->setStyleSheet("color: #4EC9B0; font-size: 12px; font-weight: "
                              "bold; font-family: 'Consolas';");
  cpuLay->addWidget(cpuNameLabel, 0, 2, 1, 2);

  cpuLay->addWidget(createKeyLabel("Cores / Threads"), 1, 1);
  cpuCoresLabel =
      createValueLabel(hardwareDetected ? QString("%1C / %2T")
                                              .arg(sysInfo.cpu.physicalCores)
                                              .arg(sysInfo.cpu.logicalCores)
                                        : "-");
  cpuLay->addWidget(cpuCoresLabel, 1, 2);

  cpuLay->addWidget(createKeyLabel("Clock"), 1, 3);
  cpuClockLabel =
      createValueLabel(hardwareDetected ? QString("%1 MHz (Boost: %2 MHz)")
                                              .arg(sysInfo.cpu.baseClockMHz)
                                              .arg(sysInfo.cpu.maxClockMHz)
                                        : "-");
  cpuLay->addWidget(cpuClockLabel, 1, 4);

  cpuLay->addWidget(createKeyLabel("Cache"), 2, 1);
  cpuCacheLabel =
      createValueLabel(hardwareDetected ? QString("L2: %1 KB | L3: %2 MB")
                                              .arg(sysInfo.cpu.l2CacheKB)
                                              .arg(sysInfo.cpu.l3CacheMB)
                                        : "-");
  cpuLay->addWidget(cpuCacheLabel, 2, 2);

  cpuLay->addWidget(createKeyLabel("Features"), 2, 3);
  QStringList feats;
  if (hardwareDetected) {
    if (sysInfo.cpu.hasAVX2)
      feats << "AVX2";
    if (sysInfo.cpu.hasAVX512)
      feats << "AVX-512";
    if (sysInfo.cpu.isAMDRyzenAI)
      feats << "Ryzen AI";
    if (sysInfo.cpu.isIntelAIBoost)
      feats << "AI Boost";
  }
  cpuFeaturesLabel = createValueLabel(feats.isEmpty() ? "-" : feats.join(", "));
  cpuLay->addWidget(cpuFeaturesLabel, 2, 4);

  // Live row
  cpuLay->addWidget(createKeyLabel("Temperature"), 3, 1);
  cpuTempLabel = createValueLabel("--");
  cpuLay->addWidget(cpuTempLabel, 3, 2);
  cpuLay->addWidget(createKeyLabel("Usage"), 3, 3);
  cpuUsageLabel = createValueLabel("--");
  cpuLay->addWidget(cpuUsageLabel, 3, 4);
  cpuLay->addWidget(createKeyLabel("Power"), 4, 1);
  cpuPowerLabel = createValueLabel("--");
  cpuLay->addWidget(cpuPowerLabel, 4, 2);
  cpuLay->addWidget(createKeyLabel("Voltage"), 4, 3);
  cpuVoltageLabel = createValueLabel("--");
  cpuLay->addWidget(cpuVoltageLabel, 4, 4);

  cpuLay->setColumnStretch(2, 1);
  cpuLay->setColumnStretch(4, 1);
  grid->addWidget(cpuGroup, 0, 0, 1, 2);

  // --- Mini Charts ---
  auto *chartsGroup =
      createInfoGroup("Real-Time Monitoring", QColor("#4EC9B0"));
  auto *chartsLay = new QGridLayout(chartsGroup);
  chartsLay->setSpacing(4);
  chartsLay->setContentsMargins(4, 4, 4, 4);

  cpuTempSeries = new QLineSeries();
  cpuTempSeries->setPen(QPen(QColor("#FF8C00"), 1.5));
  cpuTempChart = createMiniChart("CPU Temp (°C)", QColor("#FF8C00"));
  cpuTempChart->chart()->removeSeries(cpuTempChart->chart()->series().first());
  cpuTempChart->chart()->addSeries(cpuTempSeries);
  cpuTempSeries->attachAxis(
      cpuTempChart->chart()->axes(Qt::Horizontal).first());
  cpuTempSeries->attachAxis(cpuTempChart->chart()->axes(Qt::Vertical).first());
  chartsLay->addWidget(cpuTempChart, 0, 0);

  cpuUsageSeries = new QLineSeries();
  cpuUsageSeries->setPen(QPen(QColor("#569CD6"), 1.5));
  cpuUsageChart = createMiniChart("CPU Usage (%)", QColor("#569CD6"));
  cpuUsageChart->chart()->removeSeries(
      cpuUsageChart->chart()->series().first());
  cpuUsageChart->chart()->addSeries(cpuUsageSeries);
  cpuUsageSeries->attachAxis(
      cpuUsageChart->chart()->axes(Qt::Horizontal).first());
  cpuUsageSeries->attachAxis(
      cpuUsageChart->chart()->axes(Qt::Vertical).first());
  chartsLay->addWidget(cpuUsageChart, 0, 1);

  gpuTempSeries = new QLineSeries();
  gpuTempSeries->setPen(QPen(QColor("#76B900"), 1.5));
  gpuTempChart = createMiniChart("GPU Temp (°C)", QColor("#76B900"));
  gpuTempChart->chart()->removeSeries(gpuTempChart->chart()->series().first());
  gpuTempChart->chart()->addSeries(gpuTempSeries);
  gpuTempSeries->attachAxis(
      gpuTempChart->chart()->axes(Qt::Horizontal).first());
  gpuTempSeries->attachAxis(gpuTempChart->chart()->axes(Qt::Vertical).first());
  chartsLay->addWidget(gpuTempChart, 1, 0);

  gpuUsageSeries = new QLineSeries();
  gpuUsageSeries->setPen(QPen(QColor("#CE9178"), 1.5));
  gpuUsageChart = createMiniChart("GPU Usage (%)", QColor("#CE9178"));
  gpuUsageChart->chart()->removeSeries(
      gpuUsageChart->chart()->series().first());
  gpuUsageChart->chart()->addSeries(gpuUsageSeries);
  gpuUsageSeries->attachAxis(
      gpuUsageChart->chart()->axes(Qt::Horizontal).first());
  gpuUsageSeries->attachAxis(
      gpuUsageChart->chart()->axes(Qt::Vertical).first());
  chartsLay->addWidget(gpuUsageChart, 1, 1);

  grid->addWidget(chartsGroup, 1, 0, 1, 2);

  // --- GPU Panel(s) ---
  int gridRow = 2;
  if (hardwareDetected) {
    for (size_t i = 0; i < sysInfo.gpus.size(); ++i) {
      const auto &gpu = sysInfo.gpus[i];
      bool isNV = (gpu.vendor == "NVIDIA" ||
                   gpu.name.find("NVIDIA") != std::string::npos ||
                   gpu.name.find("GeForce") != std::string::npos);
      QColor bc = isNV ? QColor("#76B900") : QColor("#ED1C24");
      auto *gpuGroup = createInfoGroup(QString("GPU #%1").arg(i), bc);
      auto *gl = new QGridLayout(gpuGroup);
      gl->setSpacing(3);
      gl->setContentsMargins(6, 4, 6, 4);
      GPUSummaryLabels labels;

      gl->addWidget(createVendorLogo(isNV ? "NVIDIA" : "AMD"), 0, 0, 2, 1);
      gl->addWidget(createKeyLabel("Name"), 0, 1);
      labels.name = createValueLabel(QString::fromStdString(gpu.name));
      labels.name->setStyleSheet("color: #CE9178; font-size: 12px; "
                                 "font-weight: bold; font-family: 'Consolas';");
      gl->addWidget(labels.name, 0, 2, 1, 3);

      gl->addWidget(createKeyLabel("VRAM"), 1, 1);
      float vGB = gpu.vramBytes / (1024.0f * 1024.0f * 1024.0f);
      labels.vram = createValueLabel(
          vGB >= 1.0f ? QString("%1 GB").arg(vGB, 0, 'f', 1)
                      : QString("%1 MB").arg(
                            gpu.vramBytes / (1024.0f * 1024.0f), 0, 'f', 0));
      gl->addWidget(labels.vram, 1, 2);
      gl->addWidget(createKeyLabel("Driver"), 1, 3);
      labels.driver =
          createValueLabel(QString::fromStdString(gpu.driverVersion));
      gl->addWidget(labels.driver, 1, 4);

      gl->addWidget(createKeyLabel("Temp"), 2, 1);
      labels.temp = createValueLabel("--");
      gl->addWidget(labels.temp, 2, 2);
      gl->addWidget(createKeyLabel("Usage"), 2, 3);
      labels.usage = createValueLabel("--");
      gl->addWidget(labels.usage, 2, 4);
      gl->addWidget(createKeyLabel("Power"), 3, 1);
      labels.power = createValueLabel("--");
      gl->addWidget(labels.power, 3, 2);
      gl->addWidget(createKeyLabel("Core Clock"), 3, 3);
      labels.clock = createValueLabel("--");
      gl->addWidget(labels.clock, 3, 4);
      gl->addWidget(createKeyLabel("Mem Clock"), 4, 1);
      labels.memClock = createValueLabel("--");
      gl->addWidget(labels.memClock, 4, 2);
      gl->addWidget(createKeyLabel("VRAM Used"), 4, 3);
      labels.vramUsed = createValueLabel("--");
      gl->addWidget(labels.vramUsed, 4, 4);

      labels.shared = nullptr;
      labels.fanSpeed = nullptr;
      gl->setColumnStretch(2, 1);
      gl->setColumnStretch(4, 1);
      int col = (int)(i % 2);
      grid->addWidget(gpuGroup, gridRow, col);
      if (col == 1 || i == sysInfo.gpus.size() - 1)
        gridRow++;
      gpuLabels.append(labels);
    }
  }

  // --- Memory Panel ---
  auto *memGroup = createInfoGroup("Memory", QColor("#6A9955"));
  auto *ml = new QGridLayout(memGroup);
  ml->setSpacing(3);
  ml->setContentsMargins(6, 4, 6, 4);
  ml->addWidget(createKeyLabel("Total"), 0, 0);
  memTotalLabel = createValueLabel("--");
  memTotalLabel->setStyleSheet("color: #4EC9B0; font-size: 12px; font-weight: "
                               "bold; font-family: 'Consolas';");
  ml->addWidget(memTotalLabel, 0, 1);
  ml->addWidget(createKeyLabel("Type"), 0, 2);
  memTypeLabel = createValueLabel(hardwareDetected && !sysInfo.ram.type.empty()
                                      ? QString::fromStdString(sysInfo.ram.type)
                                      : "-");
  ml->addWidget(memTypeLabel, 0, 3);
  ml->addWidget(createKeyLabel("Used"), 1, 0);
  memUsedLabel = createValueLabel("--");
  ml->addWidget(memUsedLabel, 1, 1);
  ml->addWidget(createKeyLabel("Speed"), 1, 2);
  memSpeedLabel =
      createValueLabel(hardwareDetected && sysInfo.ram.speedMHz > 0
                           ? QString("%1 MHz").arg(sysInfo.ram.speedMHz)
                           : "-");
  ml->addWidget(memSpeedLabel, 1, 3);
  ml->addWidget(createKeyLabel("Available"), 2, 0);
  memAvailLabel = createValueLabel("--");
  ml->addWidget(memAvailLabel, 2, 1);
  ml->addWidget(createKeyLabel("Usage"), 2, 2);
  memUsageLabel = createValueLabel("--");
  ml->addWidget(memUsageLabel, 2, 3);
  ml->setColumnStretch(1, 1);
  ml->setColumnStretch(3, 1);
  grid->addWidget(memGroup, gridRow, 0);

  // --- OS Panel ---
  auto *osGroup = createInfoGroup("System", QColor("#808080"));
  auto *ol = new QGridLayout(osGroup);
  ol->setSpacing(3);
  ol->setContentsMargins(6, 4, 6, 4);
  ol->addWidget(createKeyLabel("OS"), 0, 0);
  osLabel = createValueLabel(
      hardwareDetected ? QString::fromStdString(sysInfo.osVersion) : "-");
  ol->addWidget(osLabel, 0, 1);
  ol->setColumnStretch(1, 1);
  grid->addWidget(osGroup, gridRow, 1);
  gridRow++;

  // --- Storage Panel ---
  if (hardwareDetected && !sysInfo.storage.empty()) {
    auto *sg = createInfoGroup("Drives", QColor("#D7BA7D"));
    auto *sl = new QGridLayout(sg);
    sl->setSpacing(3);
    sl->setContentsMargins(6, 4, 6, 4);
    auto mkHdr = [](const QString &t) {
      auto *l = new QLabel(t);
      l->setStyleSheet("color: #D7BA7D; font-size: 11px; font-weight: bold;");
      return l;
    };
    sl->addWidget(mkHdr("Name"), 0, 0);
    sl->addWidget(mkHdr("Interface"), 0, 1);
    sl->addWidget(mkHdr("Capacity"), 0, 2);
    for (size_t i = 0; i < sysInfo.storage.size(); ++i) {
      StorageSummaryLabels lb;
      lb.name =
          createValueLabel(QString::fromStdString(sysInfo.storage[i].name));
      sl->addWidget(lb.name, (int)i + 1, 0);
      lb.type = createValueLabel(
          QString::fromStdString(sysInfo.storage[i].interfaceType) + " " +
          QString::fromStdString(sysInfo.storage[i].type));
      sl->addWidget(lb.type, (int)i + 1, 1);
      float sGB = sysInfo.storage[i].sizeBytes / (1024.0f * 1024.0f * 1024.0f);
      lb.size = createValueLabel(
          sGB >= 1000 ? QString("%1 TB").arg(sGB / 1024.0f, 0, 'f', 2)
                      : QString("%1 GB").arg(sGB, 0, 'f', 1));
      sl->addWidget(lb.size, (int)i + 1, 2);
      storageLabels.append(lb);
    }
    sl->setColumnStretch(0, 2);
    sl->setColumnStretch(1, 1);
    sl->setColumnStretch(2, 1);
    grid->addWidget(sg, gridRow, 0, 1, 2);
  }

  grid->setRowStretch(gridRow + 1, 1);
  scrollArea->setWidget(content);
  auto *tabLay = new QVBoxLayout(tab);
  tabLay->setContentsMargins(0, 0, 0, 0);
  tabLay->addWidget(scrollArea);
}

// ============================================================================
// Sensor Status Tab
// ============================================================================
void SensorPanelWidget::setupSensorTab(QWidget *tab) {
  auto *layout = new QVBoxLayout(tab);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

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

  splitter = new QSplitter(Qt::Horizontal, tab);
  hardwareTree = new QTreeWidget(tab);
  hardwareTree->setHeaderLabel("Hardware");
  hardwareTree->setMinimumWidth(260);
  hardwareTree->setMaximumWidth(340);
  hardwareTree->setIndentation(16);
  hardwareTree->setAnimated(true);
  hardwareTree->setRootIsDecorated(true);
  connect(hardwareTree, &QTreeWidget::itemClicked, this,
          &SensorPanelWidget::onTreeItemClicked);

  sensorTable = new QTableWidget(tab);
  sensorTable->setColumnCount(5);
  sensorTable->setHorizontalHeaderLabels(
      {"Sensor", "Current", "Minimum", "Maximum", "Average"});
  sensorTable->horizontalHeader()->setStretchLastSection(true);
  sensorTable->horizontalHeader()->setSectionResizeMode(0,
                                                        QHeaderView::Stretch);
  for (int c = 1; c < 5; ++c)
    sensorTable->horizontalHeader()->setSectionResizeMode(
        c, QHeaderView::ResizeToContents);
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
    #sensorTabs { background-color: #1E1E1E; border: none; }
    QTabWidget::pane { border: none; background-color: #1E1E1E; }
    QTabBar::tab {
        background-color: #2D2D30; color: #808080; border: none;
        border-bottom: 2px solid transparent; padding: 6px 16px;
        font-size: 11px; font-family: 'Segoe UI'; font-weight: bold; min-width: 120px;
    }
    QTabBar::tab:selected { color: #FFFFFF; background-color: #1E1E1E; border-bottom: 2px solid #0078D4; }
    QTabBar::tab:hover:!selected { color: #CCCCCC; background-color: #252526; }
    #sensorTopBar { background-color: #252526; border-bottom: 1px solid #3E3E42; }
    #sensorTitle { color: #CCCCCC; font-size: 12px; font-weight: bold; font-family: 'Segoe UI'; }
    #sensorStatus { color: #6A9955; font-size: 10px; font-family: 'Segoe UI'; }
    QTreeWidget {
        background-color: #1E1E1E; color: #CCCCCC; border: none;
        border-right: 1px solid #3E3E42; font-size: 11px; font-family: 'Segoe UI'; outline: none;
    }
    QTreeWidget::item { padding: 2px 6px; border: none; }
    QTreeWidget::item:selected { background-color: #094771; color: #FFFFFF; }
    QTreeWidget::item:hover { background-color: #2A2D2E; }
    QTreeWidget::branch { background-color: #1E1E1E; }
    QHeaderView::section {
        background-color: #252526; color: #CCCCCC; border: none;
        border-bottom: 1px solid #3E3E42; padding: 3px 8px; font-size: 10px; font-weight: bold;
    }
    QTableWidget {
        background-color: #1E1E1E; color: #CCCCCC; border: none; gridline-color: #2D2D30;
        font-size: 11px; font-family: 'Consolas'; outline: none; selection-background-color: #094771;
    }
    QTableWidget::item { padding: 1px 8px; border-bottom: 1px solid #252526; }
    QTableWidget::item:selected { background-color: #094771; color: #FFFFFF; }
    QTableWidget::item:alternate { background-color: #1A1A1A; }
    QScrollBar:vertical { background-color: #1E1E1E; width: 8px; border: none; }
    QScrollBar::handle:vertical { background-color: #424242; border-radius: 4px; min-height: 20px; }
    QScrollBar::handle:vertical:hover { background-color: #4F4F4F; }
    QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
    QScrollBar:horizontal { background-color: #1E1E1E; height: 8px; border: none; }
    QScrollBar::handle:horizontal { background-color: #424242; border-radius: 4px; min-width: 20px; }
    QSplitter::handle { background-color: #3E3E42; width: 1px; }
  )");
}

// ============================================================================
// Hardware Tree
// ============================================================================
void SensorPanelWidget::populateHardwareTree() {
  hardwareTree->clear();
  auto addSub = [](QTreeWidgetItem *p, const QString &t, const QString &cat,
                   const QColor &c) {
    auto *i = new QTreeWidgetItem(p);
    i->setText(0, t);
    i->setData(0, Qt::UserRole, cat);
    i->setForeground(0, c);
  };

  auto *sys = new QTreeWidgetItem(hardwareTree);
  sys->setText(0, hardwareDetected
                      ? "System: " + QString::fromStdString(sysInfo.cpu.name)
                      : "System");
  sys->setData(0, Qt::UserRole, "All");
  sys->setForeground(0, QColor("#4EC9B0"));
  sys->setExpanded(true);

  auto *cpu = new QTreeWidgetItem(sys);
  cpu->setText(0, "CPU: " + (hardwareDetected
                                 ? QString::fromStdString(sysInfo.cpu.name)
                                 : "CPU"));
  cpu->setData(0, Qt::UserRole, "CPU");
  cpu->setForeground(0, QColor("#569CD6"));
  cpu->setExpanded(true);
  addSub(cpu, "Temperatures", "CPU_Temp", QColor("#CE9178"));
  addSub(cpu, "Clocks", "CPU_Clock", QColor("#4EC9B0"));
  addSub(cpu, "Utilization", "CPU_Load", QColor("#DCDCAA"));
  addSub(cpu, "Power / Voltage", "CPU_Power", QColor("#D7BA7D"));

  if (hardwareDetected && !sysInfo.gpus.empty()) {
    for (size_t i = 0; i < sysInfo.gpus.size(); ++i) {
      auto *g = new QTreeWidgetItem(sys);
      g->setText(0, "GPU: " + QString::fromStdString(sysInfo.gpus[i].name));
      g->setData(0, Qt::UserRole, QString("GPU_%1").arg(i));
      g->setForeground(0, QColor("#CE9178"));
      g->setExpanded(true);
      QString pfx = QString("GPU_%1_").arg(i);
      addSub(g, "Temperatures", pfx + "Temp", QColor("#CE9178"));
      addSub(g, "Clocks", pfx + "Clock", QColor("#4EC9B0"));
      addSub(g, "Utilization", pfx + "Load", QColor("#DCDCAA"));
      addSub(g, "Power", pfx + "Power", QColor("#D7BA7D"));
      addSub(g, "Fans", pfx + "Fan", QColor("#608B4E"));
      addSub(g, "Memory", pfx + "Mem", QColor("#569CD6"));
    }
  }

  auto *mem = new QTreeWidgetItem(sys);
  mem->setText(0, "Memory");
  mem->setData(0, Qt::UserRole, "RAM");
  mem->setForeground(0, QColor("#6A9955"));

  if (hardwareDetected && !sysInfo.storage.empty()) {
    auto *stg = new QTreeWidgetItem(sys);
    stg->setText(0, "Storage");
    stg->setData(0, Qt::UserRole, "Storage");
    stg->setForeground(0, QColor("#D7BA7D"));
    for (size_t i = 0; i < sysInfo.storage.size(); ++i)
      addSub(stg, QString::fromStdString(sysInfo.storage[i].name),
             QString("Storage_%1").arg(i), QColor("#808080"));
  }
  hardwareTree->setCurrentItem(sys);
}

void SensorPanelWidget::onTreeItemClicked(QTreeWidgetItem *item, int column) {
  Q_UNUSED(column);
  currentCategory = item->data(0, Qt::UserRole).toString();
  populateSensorTable(currentCategory);
}

// ============================================================================
// Table Population
// ============================================================================
void SensorPanelWidget::populateSensorTable(const QString &category) {
  sensorTable->setRowCount(0);
  QString lastSection;

  for (int i = 0; i < sensorRecords.size(); ++i) {
    const auto &rec = sensorRecords[i];
    bool show = false;
    if (category == "All")
      show = true;
    else if (category.startsWith("CPU") && rec.category.startsWith("CPU")) {
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
        QString pfx = category.left(5);
        if (!rec.category.startsWith(pfx))
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
        else if (category.count('_') == 1)
          show = true;
      } else
        show = true;
    } else if (category == "RAM" && rec.category == "RAM")
      show = true;
    else if (category.startsWith("Storage") &&
             rec.category.startsWith("Storage"))
      show = true;
    if (!show)
      continue;

    // Section header in "All" view
    if (category == "All" && rec.category != lastSection) {
      lastSection = rec.category;
      int hr = sensorTable->rowCount();
      sensorTable->insertRow(hr);
      QString sn = rec.category;
      if (rec.category == "CPU" && hardwareDetected)
        sn = "CPU: " + QString::fromStdString(sysInfo.cpu.name);
      else if (rec.category.startsWith("GPU_") && hardwareDetected) {
        int idx = rec.category.mid(4).toInt();
        if (idx < (int)sysInfo.gpus.size())
          sn = "GPU: " + QString::fromStdString(sysInfo.gpus[idx].name);
      }
      auto *hi = new QTableWidgetItem(QString("  %1").arg(sn));
      hi->setForeground(QColor("#4EC9B0"));
      hi->setBackground(QColor("#252526"));
      hi->setFont(QFont("Segoe UI", 10, QFont::Bold));
      sensorTable->setItem(hr, 0, hi);
      for (int c = 1; c < 5; ++c) {
        auto *s = new QTableWidgetItem("");
        s->setBackground(QColor("#252526"));
        sensorTable->setItem(hr, c, s);
      }
    }

    int row = sensorTable->rowCount();
    sensorTable->insertRow(row);

    auto *nameItem = new QTableWidgetItem(rec.name);
    nameItem->setForeground(QColor("#CCCCCC"));
    sensorTable->setItem(row, 0, nameItem);

    int dec = 1;
    if (rec.unit == "MHz" || rec.unit == "RPM" || rec.unit == "MB")
      dec = 0;
    else if (rec.unit == "W" || rec.unit == "V")
      dec = 2;
    else if (rec.unit == "GB")
      dec = 2;

    auto mkItem = [&](float val) {
      auto *it = new QTableWidgetItem(
          QString("%1 %2").arg(val, 0, 'f', dec).arg(rec.unit));
      it->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
      return it;
    };

    auto *cur = mkItem(rec.current);
    if (rec.sensorType == "Temperature")
      cur->setForeground(QColor(tempColor(rec.current)));
    else if (rec.sensorType == "Load")
      cur->setForeground(QColor(loadColor(rec.current)));
    else if (rec.sensorType == "Clock")
      cur->setForeground(QColor("#4EC9B0"));
    else if (rec.sensorType == "Power")
      cur->setForeground(QColor("#D7BA7D"));
    else if (rec.sensorType == "Fan")
      cur->setForeground(QColor("#608B4E"));
    else if (rec.sensorType == "Voltage")
      cur->setForeground(QColor("#B5CEA8"));
    else
      cur->setForeground(QColor("#DCDCAA"));
    sensorTable->setItem(row, 1, cur);

    auto *mn = mkItem(rec.min < 999999 ? rec.min : 0);
    mn->setForeground(QColor("#569CD6"));
    sensorTable->setItem(row, 2, mn);

    auto *mx = mkItem(rec.max > -999999 ? rec.max : 0);
    mx->setForeground(QColor("#CE9178"));
    sensorTable->setItem(row, 3, mx);

    auto *av = mkItem(rec.count > 0 ? rec.sum / rec.count : 0);
    av->setForeground(QColor("#808080"));
    sensorTable->setItem(row, 4, av);
  }
  statusLabel->setText(
      QString("%1 sensors | Updated").arg(sensorRecords.size()));
}

// ============================================================================
// Update Summary
// ============================================================================
void SensorPanelWidget::updateSummaryTab() {
  MEMORYSTATUSEX ms;
  ms.dwLength = sizeof(ms);
  if (GlobalMemoryStatusEx(&ms)) {
    float tGB = (float)ms.ullTotalPhys / (1024.0f * 1024.0f * 1024.0f);
    float uGB = (float)(ms.ullTotalPhys - ms.ullAvailPhys) /
                (1024.0f * 1024.0f * 1024.0f);
    float aGB = (float)ms.ullAvailPhys / (1024.0f * 1024.0f * 1024.0f);
    memTotalLabel->setText(QString("%1 GB").arg(tGB, 0, 'f', 2));
    memUsedLabel->setText(QString("%1 GB").arg(uGB, 0, 'f', 2));
    memAvailLabel->setText(QString("%1 GB").arg(aGB, 0, 'f', 2));
    memUsageLabel->setText(QString("%1 %").arg(ms.dwMemoryLoad));
    memUsageLabel->setStyleSheet(
        QString("color: %1; font-size: 11px; font-family: 'Consolas';")
            .arg(loadColor((float)ms.dwMemoryLoad)));
  }

  float cpuT = 0, cpuU = 0, gpuT = 0, gpuU = 0;
  for (const auto &r : sensorRecords) {
    if (r.category == "CPU") {
      if (r.name == "CPU Package" && r.sensorType == "Temperature") {
        cpuTempLabel->setText(QString("%1 C").arg(r.current, 0, 'f', 1));
        cpuTempLabel->setStyleSheet(
            QString("color: %1; font-size: 11px; font-family: 'Consolas';")
                .arg(tempColor(r.current)));
        cpuT = r.current;
      } else if (r.name == "Total CPU Usage") {
        cpuUsageLabel->setText(QString("%1 %").arg(r.current, 0, 'f', 1));
        cpuUsageLabel->setStyleSheet(
            QString("color: %1; font-size: 11px; font-family: 'Consolas';")
                .arg(loadColor(r.current)));
        cpuU = r.current;
      } else if (r.name == "Package Power") {
        cpuPowerLabel->setText(QString("%1 W").arg(r.current, 0, 'f', 2));
      } else if (r.name == "CPU Voltage") {
        cpuVoltageLabel->setText(QString("%1 V").arg(r.current, 0, 'f', 3));
      }
    }
  }

  for (int g = 0; g < gpuLabels.size(); ++g) {
    auto &lb = gpuLabels[g];
    QString gc = QString("GPU_%1").arg(g);
    for (const auto &r : sensorRecords) {
      if (r.category != gc)
        continue;
      if (r.name == "GPU Temperature") {
        lb.temp->setText(QString("%1 C").arg(r.current, 0, 'f', 1));
        lb.temp->setStyleSheet(
            QString("color: %1; font-size: 11px; font-family: 'Consolas';")
                .arg(tempColor(r.current)));
        if (g == 0)
          gpuT = r.current;
      } else if (r.name == "GPU Usage" || r.name == "GPU Utilization") {
        lb.usage->setText(QString("%1 %").arg(r.current, 0, 'f', 1));
        lb.usage->setStyleSheet(
            QString("color: %1; font-size: 11px; font-family: 'Consolas';")
                .arg(loadColor(r.current)));
        if (g == 0)
          gpuU = r.current;
      } else if (r.name == "GPU Power")
        lb.power->setText(QString("%1 W").arg(r.current, 0, 'f', 2));
      else if (r.name == "GPU Core Clock")
        lb.clock->setText(QString("%1 MHz").arg(r.current, 0, 'f', 0));
      else if (r.name == "GPU Memory Clock")
        lb.memClock->setText(QString("%1 MHz").arg(r.current, 0, 'f', 0));
      else if (r.name == "VRAM Used")
        lb.vramUsed->setText(QString("%1 MB").arg(r.current, 0, 'f', 0));
    }
  }

  // Update charts
  updateMiniChart(cpuTempChart, cpuTempSeries, cpuT);
  updateMiniChart(cpuUsageChart, cpuUsageSeries, cpuU);
  updateMiniChart(gpuTempChart, gpuTempSeries, gpuT);
  updateMiniChart(gpuUsageChart, gpuUsageSeries, gpuU);
  chartSampleCount++;
}

// ============================================================================
// Main Update — comprehensive per-core sensors
// ============================================================================
void SensorPanelWidget::updateData() {
  // === CPU Sensors ===
  if (sensorMonitor) {
    auto cs = sensorMonitor->ReadCPUSensorsWMI();
    if (cs.packageTemp > 0)
      addOrUpdateRecord("CPU Package", "CPU", cs.packageTemp, "°C",
                        "Temperature");
    if (cs.packagePowerW > 0)
      addOrUpdateRecord("Package Power", "CPU", cs.packagePowerW, "W", "Power");
    if (cs.voltage > 0)
      addOrUpdateRecord("CPU Voltage", "CPU", cs.voltage, "V", "Voltage");
    if (cs.corePowerW > 0)
      addOrUpdateRecord("Core Power", "CPU", cs.corePowerW, "W", "Power");
    // Per-core temperatures
    for (uint32_t c = 0; c < cs.coreCount && c < 64; ++c) {
      if (cs.coreTemps[c] > 0)
        addOrUpdateRecord(QString("Core %1 Temp").arg(c), "CPU",
                          cs.coreTemps[c], "°C", "Temperature");
    }
  }

  // CPU total usage
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
      uint64_t td = (kernel - prevKernel) + (user - prevUser);
      uint64_t id = idle - prevIdle;
      if (td > 0)
        addOrUpdateRecord("Total CPU Usage", "CPU",
                          (float)(100.0 * (1.0 - (double)id / (double)td)), "%",
                          "Load");
    }
    prevIdle = idle;
    prevKernel = kernel;
    prevUser = user;
  }

  // Per-core CPU usage via PDH
  {
    static PDH_HQUERY coreQuery = nullptr;
    static QVector<PDH_HCOUNTER> coreCounters;
    static bool coreInit = false;
    int numCores = hardwareDetected ? sysInfo.cpu.logicalCores : 0;
    if (!coreInit && numCores > 0) {
      coreInit = true;
      if (PdhOpenQueryW(nullptr, 0, &coreQuery) == ERROR_SUCCESS) {
        for (int c = 0; c < numCores && c < 64; ++c) {
          PDH_HCOUNTER hc;
          QString path = QString("\\Processor(%1)\\% Processor Time").arg(c);
          if (PdhAddEnglishCounterW(coreQuery, path.toStdWString().c_str(), 0,
                                    &hc) == ERROR_SUCCESS)
            coreCounters.append(hc);
          else
            coreCounters.append(nullptr);
        }
        PdhCollectQueryData(coreQuery);
      }
    }
    if (coreQuery && !coreCounters.isEmpty()) {
      PdhCollectQueryData(coreQuery);
      for (int c = 0; c < coreCounters.size(); ++c) {
        if (!coreCounters[c])
          continue;
        PDH_FMT_COUNTERVALUE val;
        if (PdhGetFormattedCounterValue(coreCounters[c], PDH_FMT_DOUBLE,
                                        nullptr, &val) == ERROR_SUCCESS) {
          float usage = (float)val.doubleValue;
          if (usage >= 0 && usage <= 100)
            addOrUpdateRecord(QString("Core %1 Usage").arg(c), "CPU", usage,
                              "%", "Load");
        }
      }
    }
  }

  // CPU clock
  if (hardwareDetected && sysInfo.cpu.maxClockMHz > 0)
    addOrUpdateRecord("CPU Clock", "CPU", (float)sysInfo.cpu.maxClockMHz, "MHz",
                      "Clock");

  // === GPU Sensors ===
  int nvidiaGpuIdx = -1, amdGpuIdx = -1;
  if (hardwareDetected) {
    for (size_t i = 0; i < sysInfo.gpus.size(); ++i) {
      if (sysInfo.gpus[i].vendor == "NVIDIA" ||
          sysInfo.gpus[i].name.find("NVIDIA") != std::string::npos ||
          sysInfo.gpus[i].name.find("GeForce") != std::string::npos)
        nvidiaGpuIdx = (int)i;
      else if (sysInfo.gpus[i].vendor == "AMD" ||
               sysInfo.gpus[i].name.find("AMD") != std::string::npos ||
               sysInfo.gpus[i].name.find("Radeon") != std::string::npos)
        amdGpuIdx = (int)i;
    }
  }

  if (nvmlAvailable && sensorMonitor && nvidiaGpuIdx >= 0) {
    auto gs = sensorMonitor->ReadNVIDIASensors();
    QString gc = QString("GPU_%1").arg(nvidiaGpuIdx);
    if (gs.temperature > 0)
      addOrUpdateRecord("GPU Temperature", gc, gs.temperature, "°C",
                        "Temperature");
    if (gs.gpuClock > 0)
      addOrUpdateRecord("GPU Core Clock", gc, gs.gpuClock, "MHz", "Clock");
    if (gs.memoryClock > 0)
      addOrUpdateRecord("GPU Memory Clock", gc, gs.memoryClock, "MHz", "Clock");
    if (gs.powerW > 0)
      addOrUpdateRecord("GPU Power", gc, gs.powerW, "W", "Power");
    if (gs.powerLimitW > 0)
      addOrUpdateRecord("GPU Power Limit", gc, gs.powerLimitW, "W", "Power");
    if (gs.fanSpeedPercent > 0)
      addOrUpdateRecord("GPU Fan", gc, gs.fanSpeedPercent, "%", "Fan");
    if (gs.fanSpeedRPM > 0)
      addOrUpdateRecord("GPU Fan Speed", gc, gs.fanSpeedRPM, "RPM", "Fan");
    addOrUpdateRecord("GPU Usage", gc, gs.usagePercent, "%", "Load");
    if (gs.memoryTotalMB > 0) {
      addOrUpdateRecord("VRAM Used", gc, gs.memoryUsedMB, "MB", "Data");
      addOrUpdateRecord("VRAM Total", gc, gs.memoryTotalMB, "MB", "Data");
      addOrUpdateRecord("VRAM Usage", gc,
                        (gs.memoryUsedMB / gs.memoryTotalMB) * 100.0f, "%",
                        "Load");
    }
  }

  if (amdGpuIdx >= 0 && hardwareDetected &&
      amdGpuIdx < (int)sysInfo.gpus.size()) {
    QString gc = QString("GPU_%1").arg(amdGpuIdx);
    float dvMB = sysInfo.gpus[amdGpuIdx].vramBytes / (1024.0f * 1024.0f);
    if (dvMB > 0)
      addOrUpdateRecord("Dedicated VRAM", gc, dvMB, "MB", "Data");
    float sMB = sysInfo.gpus[amdGpuIdx].sharedMemBytes / (1024.0f * 1024.0f);
    if (sMB > 0)
      addOrUpdateRecord("Shared GPU Memory", gc, sMB, "MB", "Data");

    static PDH_HQUERY gpuQuery = nullptr;
    static PDH_HCOUNTER gpuCounter = nullptr;
    static bool pdhInit = false, pdhOk = false;
    if (!pdhInit) {
      pdhInit = true;
      if (PdhOpenQueryW(nullptr, 0, &gpuQuery) == ERROR_SUCCESS)
        if (PdhAddEnglishCounterW(gpuQuery,
                                  L"\\GPU Engine(*)\\Utilization Percentage", 0,
                                  &gpuCounter) == ERROR_SUCCESS) {
          PdhCollectQueryData(gpuQuery);
          pdhOk = true;
        }
    }
    if (pdhOk && gpuQuery) {
      PdhCollectQueryData(gpuQuery);
      PDH_FMT_COUNTERVALUE cv;
      if (PdhGetFormattedCounterValue(gpuCounter, PDH_FMT_DOUBLE, nullptr,
                                      &cv) == ERROR_SUCCESS) {
        float u = (float)cv.doubleValue;
        if (u >= 0 && u <= 100)
          addOrUpdateRecord("GPU Utilization", gc, u, "%", "Load");
      }
    }
  }

  // === RAM Sensors ===
  {
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    if (GlobalMemoryStatusEx(&ms)) {
      float tGB = (float)ms.ullTotalPhys / (1024.0f * 1024.0f * 1024.0f);
      float uGB = (float)(ms.ullTotalPhys - ms.ullAvailPhys) /
                  (1024.0f * 1024.0f * 1024.0f);
      addOrUpdateRecord("Physical Memory Used", "RAM", uGB, "GB", "Data");
      addOrUpdateRecord("Physical Memory Total", "RAM", tGB, "GB", "Data");
      addOrUpdateRecord("Physical Memory Load", "RAM", (float)ms.dwMemoryLoad,
                        "%", "Load");
      addOrUpdateRecord("Available Memory", "RAM",
                        (float)ms.ullAvailPhys / (1024.0f * 1024.0f * 1024.0f),
                        "GB", "Data");
      addOrUpdateRecord("Virtual Memory Used", "RAM",
                        (float)(ms.ullTotalVirtual - ms.ullAvailVirtual) /
                            (1024.0f * 1024.0f * 1024.0f),
                        "GB", "Data");
      addOrUpdateRecord("Virtual Memory Total", "RAM",
                        (float)ms.ullTotalVirtual /
                            (1024.0f * 1024.0f * 1024.0f),
                        "GB", "Data");
      addOrUpdateRecord("Page File Used", "RAM",
                        (float)(ms.ullTotalPageFile - ms.ullAvailPageFile) /
                            (1024.0f * 1024.0f * 1024.0f),
                        "GB", "Data");
      addOrUpdateRecord("Page File Total", "RAM",
                        (float)ms.ullTotalPageFile /
                            (1024.0f * 1024.0f * 1024.0f),
                        "GB", "Data");
    }
    if (hardwareDetected && sysInfo.ram.speedMHz > 0)
      addOrUpdateRecord("Memory Clock", "RAM", (float)sysInfo.ram.speedMHz,
                        "MHz", "Clock");
  }

  updateSummaryTab();
  populateSensorTable(currentCategory);
}

// ============================================================================
// Record Helpers & Color Helpers
// ============================================================================
int SensorPanelWidget::findRecord(const QString &name,
                                  const QString &category) {
  for (int i = 0; i < sensorRecords.size(); ++i)
    if (sensorRecords[i].name == name && sensorRecords[i].category == category)
      return i;
  return -1;
}

void SensorPanelWidget::addOrUpdateRecord(const QString &name,
                                          const QString &category, float value,
                                          const QString &unit,
                                          const QString &sensorType) {
  int idx = findRecord(name, category);
  if (idx < 0) {
    SensorRecord r;
    r.name = name;
    r.category = category;
    r.current = value;
    r.min = value;
    r.max = value;
    r.sum = value;
    r.count = 1;
    r.unit = unit;
    r.sensorType = sensorType;
    sensorRecords.append(r);
  } else {
    auto &r = sensorRecords[idx];
    r.current = value;
    if (value < r.min)
      r.min = value;
    if (value > r.max)
      r.max = value;
    r.sum += value;
    r.count++;
  }
}

QString SensorPanelWidget::tempColor(float t) {
  if (t >= 90)
    return "#FF4444";
  if (t >= 75)
    return "#FF8C00";
  if (t >= 60)
    return "#DCDCAA";
  return "#6A9955";
}

QString SensorPanelWidget::loadColor(float l) {
  if (l >= 90)
    return "#FF6B6B";
  if (l >= 70)
    return "#CE9178";
  if (l >= 40)
    return "#DCDCAA";
  return "#6A9955";
}
