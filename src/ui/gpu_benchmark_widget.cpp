/*
 * GPU Benchmark Widget Implementation
 * License: MIT
 */

#include "gpu_benchmark_widget.h"
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>

GPUBenchmarkWidget::GPUBenchmarkWidget(QWidget *parent)
    : QWidget(parent), m_isRunning(false) {
  setupUI();

  // Setup sensor monitor for live readings
  m_sensorMonitor = std::make_unique<SensorMonitor>();
  m_nvmlAvailable = m_sensorMonitor->InitNVML();

  // Live sensor update timer
  m_sensorTimer = new QTimer(this);
  connect(m_sensorTimer, &QTimer::timeout, this,
          &GPUBenchmarkWidget::updateLiveSensors);
  m_sensorTimer->start(1000);

  // Setup worker thread
  m_worker = new GPUBenchmarkWorker();
  m_benchmarkThread = new QThread(this);
  m_worker->moveToThread(m_benchmarkThread);

  connect(m_benchmarkThread, &QThread::finished, m_worker,
          &QObject::deleteLater);
  connect(this, &GPUBenchmarkWidget::destroyed, m_benchmarkThread,
          &QThread::quit);
  connect(m_worker, &GPUBenchmarkWorker::progressUpdate, this,
          &GPUBenchmarkWidget::onBenchmarkProgress);
  connect(m_worker, &GPUBenchmarkWorker::benchmarkComplete, this,
          &GPUBenchmarkWidget::onBenchmarkComplete);

  m_benchmarkThread->start();
}

GPUBenchmarkWidget::~GPUBenchmarkWidget() {
  m_benchmarkThread->quit();
  m_benchmarkThread->wait();
}

void GPUBenchmarkWidget::setupUI() {
  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(20, 20, 20, 20);
  mainLayout->setSpacing(20);

  // Title
  auto *titleLabel = new QLabel("GPU Benchmark");
  QFont titleFont = titleLabel->font();
  titleFont.setPointSize(18);
  titleFont.setBold(true);
  titleLabel->setFont(titleFont);
  titleLabel->setStyleSheet("color: #CE9178;"); // Amber for GPU
  mainLayout->addWidget(titleLabel);

  // Description
  auto *descLabel =
      new QLabel("Tests compute throughput (matrix operations) and "
                 "memory bandwidth with live sensor monitoring.");
  descLabel->setStyleSheet("color: #9CDCFE;");
  mainLayout->addWidget(descLabel);

  // Live GPU sensors (horizontal bar)
  auto *sensorBar = new QFrame(this);
  sensorBar->setObjectName("gpuSensorBar");
  sensorBar->setStyleSheet(
      "#gpuSensorBar { background-color: #2D2D30; border: 1px solid #3E3E42; "
      "border-radius: 6px; padding: 8px; }");
  auto *sensorLayout = new QHBoxLayout(sensorBar);

  auto *sensorTitle = new QLabel("Live GPU Sensors:");
  sensorTitle->setStyleSheet("color: #808080; font-size: 11px;");
  sensorLayout->addWidget(sensorTitle);

  m_liveGpuTemp = new QLabel("Temp: -- °C");
  m_liveGpuTemp->setStyleSheet(
      "color: #FF6B6B; font-family: Consolas; font-size: 12px;");
  sensorLayout->addWidget(m_liveGpuTemp);

  m_liveGpuClock = new QLabel("Clock: -- MHz");
  m_liveGpuClock->setStyleSheet(
      "color: #4EC9B0; font-family: Consolas; font-size: 12px;");
  sensorLayout->addWidget(m_liveGpuClock);

  m_liveGpuPower = new QLabel("Power: -- W");
  m_liveGpuPower->setStyleSheet(
      "color: #D7BA7D; font-family: Consolas; font-size: 12px;");
  sensorLayout->addWidget(m_liveGpuPower);

  sensorLayout->addStretch();
  mainLayout->addWidget(sensorBar);

  // Start button
  m_startButton = new QPushButton("Start GPU Benchmark");
  m_startButton->setMinimumHeight(50);
  m_startButton->setStyleSheet(
      "QPushButton {"
      "  background-color: #CE9178;"
      "  color: white;"
      "  border: none;"
      "  border-radius: 5px;"
      "  font-size: 16px;"
      "  font-weight: bold;"
      "}"
      "QPushButton:hover { background-color: #B57B64; }"
      "QPushButton:pressed { background-color: #9A6654; }"
      "QPushButton:disabled { background-color: #3F3F46; color: #808080; }");
  connect(m_startButton, &QPushButton::clicked, this,
          &GPUBenchmarkWidget::onStartBenchmark);
  mainLayout->addWidget(m_startButton);

  // Progress bar
  m_progressBar = new QProgressBar();
  m_progressBar->setMinimum(0);
  m_progressBar->setMaximum(100);
  m_progressBar->setValue(0);
  m_progressBar->setTextVisible(true);
  m_progressBar->setStyleSheet("QProgressBar {"
                               "  border: 1px solid #3F3F46;"
                               "  border-radius: 5px;"
                               "  text-align: center;"
                               "  background-color: #2D2D30;"
                               "  color: white;"
                               "  height: 25px;"
                               "}"
                               "QProgressBar::chunk {"
                               "  background-color: #CE9178;"
                               "  border-radius: 4px;"
                               "}");
  mainLayout->addWidget(m_progressBar);

  // Status
  m_statusLabel = new QLabel("Ready to start benchmark");
  m_statusLabel->setStyleSheet("color: #9CDCFE; font-size: 13px;");
  mainLayout->addWidget(m_statusLabel);

  // Results
  m_resultsGroup = new QGroupBox("Benchmark Results");
  m_resultsGroup->setStyleSheet(
      "QGroupBox { color: #CCCCCC; border: 1px solid #3F3F46; "
      "border-radius: 5px; margin-top: 10px; padding-top: 10px; "
      "font-size: 14px; font-weight: bold; }"
      "QGroupBox::title { subcontrol-origin: margin; left: 10px; "
      "padding: 0 5px; }");

  auto *resultsLayout = new QVBoxLayout();
  resultsLayout->setSpacing(10);

  m_gpuNameLabel = new QLabel("GPU: -");
  m_gpuNameLabel->setStyleSheet("color: #DCDCAA; font-size: 12px;");
  resultsLayout->addWidget(m_gpuNameLabel);

  auto *sep1 = new QFrame();
  sep1->setFrameShape(QFrame::HLine);
  sep1->setStyleSheet("background-color: #3F3F46;");
  resultsLayout->addWidget(sep1);

  m_overallScoreLabel = new QLabel("Overall Score: -");
  m_overallScoreLabel->setStyleSheet(
      "color: #CE9178; font-size: 18px; font-weight: bold;");
  resultsLayout->addWidget(m_overallScoreLabel);

  m_computeScoreLabel = new QLabel("Compute Score: -");
  m_computeScoreLabel->setStyleSheet("color: #CCCCCC; font-size: 14px;");
  resultsLayout->addWidget(m_computeScoreLabel);

  m_memoryScoreLabel = new QLabel("Memory Score: -");
  m_memoryScoreLabel->setStyleSheet("color: #CCCCCC; font-size: 14px;");
  resultsLayout->addWidget(m_memoryScoreLabel);

  auto *sep2 = new QFrame();
  sep2->setFrameShape(QFrame::HLine);
  sep2->setStyleSheet("background-color: #3F3F46;");
  resultsLayout->addWidget(sep2);

  m_computeGflopsLabel = new QLabel("Compute: - GFLOPS");
  m_computeGflopsLabel->setStyleSheet("color: #9CDCFE; font-size: 12px;");
  resultsLayout->addWidget(m_computeGflopsLabel);

  m_memBandwidthLabel = new QLabel("Memory Bandwidth: - GB/s");
  m_memBandwidthLabel->setStyleSheet("color: #9CDCFE; font-size: 12px;");
  resultsLayout->addWidget(m_memBandwidthLabel);

  auto *sep3 = new QFrame();
  sep3->setFrameShape(QFrame::HLine);
  sep3->setStyleSheet("background-color: #3F3F46;");
  resultsLayout->addWidget(sep3);

  m_durationLabel = new QLabel("Duration: -");
  m_durationLabel->setStyleSheet(
      "color: #808080; font-size: 11px; font-style: italic;");
  resultsLayout->addWidget(m_durationLabel);

  m_resultsGroup->setLayout(resultsLayout);
  mainLayout->addWidget(m_resultsGroup);

  mainLayout->addStretch();
}

void GPUBenchmarkWidget::onStartBenchmark() {
  if (m_isRunning)
    return;

  m_isRunning = true;
  m_startButton->setEnabled(false);
  m_progressBar->setValue(0);
  m_statusLabel->setText("Initializing GPU benchmark...");

  // Reset results
  m_overallScoreLabel->setText("Overall Score: -");
  m_computeScoreLabel->setText("Compute Score: -");
  m_memoryScoreLabel->setText("Memory Score: -");
  m_computeGflopsLabel->setText("Compute: - GFLOPS");
  m_memBandwidthLabel->setText("Memory Bandwidth: - GB/s");
  m_durationLabel->setText("Duration: -");

  QMetaObject::invokeMethod(m_worker, "runBenchmark", Qt::QueuedConnection);
}

void GPUBenchmarkWidget::onBenchmarkProgress(int percent,
                                             const QString &status) {
  m_progressBar->setValue(percent);
  m_statusLabel->setText(status);
}

void GPUBenchmarkWidget::onBenchmarkComplete(const GPUBenchmarkResult &result) {
  m_isRunning = false;
  m_startButton->setEnabled(true);
  m_progressBar->setValue(100);
  m_statusLabel->setText("GPU Benchmark Complete!");
  updateResultsDisplay(result);
}

void GPUBenchmarkWidget::updateResultsDisplay(
    const GPUBenchmarkResult &result) {
  m_overallScoreLabel->setText(
      QString("Overall Score: %1").arg(result.overallScore));
  m_computeScoreLabel->setText(
      QString("Compute Score: %1").arg(result.computeScore));
  m_memoryScoreLabel->setText(
      QString("Memory Score: %1").arg(result.memoryScore));
  m_computeGflopsLabel->setText(
      QString("Compute: %1 GFLOPS").arg(result.computeGFLOPS, 0, 'f', 2));
  m_memBandwidthLabel->setText(QString("Memory Bandwidth: %1 GB/s")
                                   .arg(result.memoryBandwidthGBps, 0, 'f', 2));
  m_durationLabel->setText(
      QString("Duration: %1 seconds").arg(result.durationSeconds, 0, 'f', 2));
}

void GPUBenchmarkWidget::updateLiveSensors() {
  if (!m_nvmlAvailable || !m_sensorMonitor)
    return;

  auto gpuSensors = m_sensorMonitor->ReadNVIDIASensors();

  if (gpuSensors.temperature > 0) {
    m_liveGpuTemp->setText(
        QString("Temp: %1 °C").arg((int)gpuSensors.temperature));
  }
  if (gpuSensors.gpuClock > 0) {
    m_liveGpuClock->setText(
        QString("Clock: %1 MHz").arg((int)gpuSensors.gpuClock));
  }
  if (gpuSensors.powerW > 0) {
    m_liveGpuPower->setText(
        QString("Power: %1 W").arg(gpuSensors.powerW, 0, 'f', 1));
  }
}
