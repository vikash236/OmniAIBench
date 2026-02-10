/*
 * NPU Benchmark Widget Implementation
 * License: MIT
 */

#include "npu_benchmark_widget.h"
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>

NPUBenchmarkWidget::NPUBenchmarkWidget(QWidget *parent)
    : QWidget(parent), m_isRunning(false) {
  // Detect NPU
  m_hwDetector = std::make_unique<HardwareDetector>();
  if (m_hwDetector->Initialize()) {
    detectNPU();
  }

  setupUI();

  // Setup worker thread
  m_worker = new NPUBenchmarkWorker();
  m_benchmarkThread = new QThread(this);
  m_worker->moveToThread(m_benchmarkThread);

  connect(m_benchmarkThread, &QThread::finished, m_worker,
          &QObject::deleteLater);
  connect(this, &NPUBenchmarkWidget::destroyed, m_benchmarkThread,
          &QThread::quit);
  connect(m_worker, &NPUBenchmarkWorker::progressUpdate, this,
          &NPUBenchmarkWidget::onBenchmarkProgress);
  connect(m_worker, &NPUBenchmarkWorker::benchmarkComplete, this,
          &NPUBenchmarkWidget::onBenchmarkComplete);

  m_benchmarkThread->start();
}

NPUBenchmarkWidget::~NPUBenchmarkWidget() {
  m_benchmarkThread->quit();
  m_benchmarkThread->wait();
}

void NPUBenchmarkWidget::detectNPU() {
  auto cpuInfo = m_hwDetector->DetectCPU();

  if (cpuInfo.isAMDRyzenAI) {
    m_npuDetected = true;
    m_npuType = "AMD Ryzen AI (XDNA)";
  } else if (cpuInfo.isIntelAIBoost) {
    m_npuDetected = true;
    m_npuType = "Intel AI Boost (NPU)";
  } else {
    m_npuDetected = false;
    m_npuType = "No dedicated NPU detected";
  }
}

void NPUBenchmarkWidget::setupUI() {
  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(20, 20, 20, 20);
  mainLayout->setSpacing(20);

  // Title
  auto *titleLabel = new QLabel("NPU / AI Benchmark");
  QFont titleFont = titleLabel->font();
  titleFont.setPointSize(18);
  titleFont.setBold(true);
  titleLabel->setFont(titleFont);
  titleLabel->setStyleSheet("color: #DCDCAA;"); // Yellow for NPU
  mainLayout->addWidget(titleLabel);

  // Description
  auto *descLabel =
      new QLabel("Tests AI inference performance: dense layer computation, "
                 "convolution throughput, and latency measurement.");
  descLabel->setStyleSheet("color: #9CDCFE;");
  mainLayout->addWidget(descLabel);

  // NPU Detection status
  auto *detectionFrame = new QFrame(this);
  detectionFrame->setObjectName("npuDetection");
  detectionFrame->setStyleSheet(
      "#npuDetection { background-color: #2D2D30; border: 1px solid #3E3E42; "
      "border-radius: 6px; padding: 12px; }");
  auto *detLayout = new QHBoxLayout(detectionFrame);

  QLabel *detIcon = new QLabel(m_npuDetected ? "●" : "○", this);
  detIcon->setStyleSheet(m_npuDetected ? "color: #6A9955; font-size: 16px;"
                                       : "color: #808080; font-size: 16px;");
  detLayout->addWidget(detIcon);

  m_npuDetectionLabel = new QLabel(
      m_npuDetected
          ? QString("NPU Detected: %1").arg(m_npuType)
          : "No dedicated NPU detected — running CPU-based AI benchmark",
      this);
  m_npuDetectionLabel->setStyleSheet(
      m_npuDetected ? "color: #6A9955; font-size: 13px; font-weight: bold;"
                    : "color: #CE9178; font-size: 13px;");
  detLayout->addWidget(m_npuDetectionLabel);
  detLayout->addStretch();
  mainLayout->addWidget(detectionFrame);

  // Start button
  m_startButton = new QPushButton("Start AI Benchmark");
  m_startButton->setMinimumHeight(50);
  m_startButton->setStyleSheet(
      "QPushButton {"
      "  background-color: #DCDCAA;"
      "  color: #1E1E1E;"
      "  border: none;"
      "  border-radius: 5px;"
      "  font-size: 16px;"
      "  font-weight: bold;"
      "}"
      "QPushButton:hover { background-color: #C5C58F; }"
      "QPushButton:pressed { background-color: #AEAE74; }"
      "QPushButton:disabled { background-color: #3F3F46; color: #808080; }");
  connect(m_startButton, &QPushButton::clicked, this,
          &NPUBenchmarkWidget::onStartBenchmark);
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
                               "  background-color: #DCDCAA;"
                               "  border-radius: 4px;"
                               "}");
  mainLayout->addWidget(m_progressBar);

  // Status
  m_statusLabel = new QLabel("Ready to start AI benchmark");
  m_statusLabel->setStyleSheet("color: #9CDCFE; font-size: 13px;");
  mainLayout->addWidget(m_statusLabel);

  // Results group
  m_resultsGroup = new QGroupBox("AI Benchmark Results");
  m_resultsGroup->setStyleSheet(
      "QGroupBox { color: #CCCCCC; border: 1px solid #3F3F46; "
      "border-radius: 5px; margin-top: 10px; padding-top: 10px; "
      "font-size: 14px; font-weight: bold; }"
      "QGroupBox::title { subcontrol-origin: margin; left: 10px; "
      "padding: 0 5px; }");

  auto *resultsLayout = new QVBoxLayout();
  resultsLayout->setSpacing(10);

  auto *sep1 = new QFrame();
  sep1->setFrameShape(QFrame::HLine);
  sep1->setStyleSheet("background-color: #3F3F46;");
  resultsLayout->addWidget(sep1);

  m_overallScoreLabel = new QLabel("Overall AI Score: -");
  m_overallScoreLabel->setStyleSheet(
      "color: #DCDCAA; font-size: 18px; font-weight: bold;");
  resultsLayout->addWidget(m_overallScoreLabel);

  m_inferenceScoreLabel = new QLabel("Inference Score: -");
  m_inferenceScoreLabel->setStyleSheet("color: #CCCCCC; font-size: 14px;");
  resultsLayout->addWidget(m_inferenceScoreLabel);

  m_throughputScoreLabel = new QLabel("Throughput Score: -");
  m_throughputScoreLabel->setStyleSheet("color: #CCCCCC; font-size: 14px;");
  resultsLayout->addWidget(m_throughputScoreLabel);

  auto *sep2 = new QFrame();
  sep2->setFrameShape(QFrame::HLine);
  sep2->setStyleSheet("background-color: #3F3F46;");
  resultsLayout->addWidget(sep2);

  m_latencyLabel = new QLabel("Inference Latency: - ms");
  m_latencyLabel->setStyleSheet("color: #9CDCFE; font-size: 12px;");
  resultsLayout->addWidget(m_latencyLabel);

  m_throughputOpsLabel = new QLabel("Throughput: - inferences/sec");
  m_throughputOpsLabel->setStyleSheet("color: #9CDCFE; font-size: 12px;");
  resultsLayout->addWidget(m_throughputOpsLabel);

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

void NPUBenchmarkWidget::onStartBenchmark() {
  if (m_isRunning)
    return;

  m_isRunning = true;
  m_startButton->setEnabled(false);
  m_progressBar->setValue(0);
  m_statusLabel->setText("Initializing AI benchmark...");

  // Reset results
  m_overallScoreLabel->setText("Overall AI Score: -");
  m_inferenceScoreLabel->setText("Inference Score: -");
  m_throughputScoreLabel->setText("Throughput Score: -");
  m_latencyLabel->setText("Inference Latency: - ms");
  m_throughputOpsLabel->setText("Throughput: - inferences/sec");
  m_durationLabel->setText("Duration: -");

  QMetaObject::invokeMethod(m_worker, "runBenchmark", Qt::QueuedConnection);
}

void NPUBenchmarkWidget::onBenchmarkProgress(int percent,
                                             const QString &status) {
  m_progressBar->setValue(percent);
  m_statusLabel->setText(status);
}

void NPUBenchmarkWidget::onBenchmarkComplete(const NPUBenchmarkResult &result) {
  m_isRunning = false;
  m_startButton->setEnabled(true);
  m_progressBar->setValue(100);
  m_statusLabel->setText("AI Benchmark Complete!");
  updateResultsDisplay(result);
}

void NPUBenchmarkWidget::updateResultsDisplay(
    const NPUBenchmarkResult &result) {
  m_overallScoreLabel->setText(
      QString("Overall AI Score: %1").arg(result.overallScore));
  m_inferenceScoreLabel->setText(
      QString("Inference Score: %1").arg(result.inferenceScore));
  m_throughputScoreLabel->setText(
      QString("Throughput Score: %1").arg(result.throughputScore));
  m_latencyLabel->setText(
      QString("Inference Latency: %1 ms").arg(result.latencyMs, 0, 'f', 3));
  m_throughputOpsLabel->setText(QString("Throughput: %1 inferences/sec")
                                    .arg(result.throughputOps, 0, 'f', 0));
  m_durationLabel->setText(
      QString("Duration: %1 seconds").arg(result.durationSeconds, 0, 'f', 2));
}
