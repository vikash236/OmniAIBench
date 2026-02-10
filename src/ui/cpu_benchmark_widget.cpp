#include "cpu_benchmark_widget.h"
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>


CPUBenchmarkWidget::CPUBenchmarkWidget(QWidget *parent)
    : QWidget(parent), m_isRunning(false) {
  setupUI();

  // Setup worker thread
  m_worker = new CPUBenchmarkWorker();
  m_benchmarkThread = new QThread(this);
  m_worker->moveToThread(m_benchmarkThread);

  connect(m_benchmarkThread, &QThread::finished, m_worker,
          &QObject::deleteLater);
  connect(this, &CPUBenchmarkWidget::destroyed, m_benchmarkThread,
          &QThread::quit);
  connect(m_worker, &CPUBenchmarkWorker::progressUpdate, this,
          &CPUBenchmarkWidget::onBenchmarkProgress);
  connect(m_worker, &CPUBenchmarkWorker::benchmarkComplete, this,
          &CPUBenchmarkWidget::onBenchmarkComplete);

  m_benchmarkThread->start();
}

CPUBenchmarkWidget::~CPUBenchmarkWidget() {
  if (m_isRunning) {
    m_worker->cancelBenchmark();
  }
  m_benchmarkThread->quit();
  m_benchmarkThread->wait();
}

void CPUBenchmarkWidget::setupUI() {
  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(20, 20, 20, 20);
  mainLayout->setSpacing(20);

  // Title
  auto *titleLabel = new QLabel("CPU Benchmark");
  QFont titleFont = titleLabel->font();
  titleFont.setPointSize(18);
  titleFont.setBold(true);
  titleLabel->setFont(titleFont);
  titleLabel->setStyleSheet("color: #569CD6;"); // Blue theme for CPU
  mainLayout->addWidget(titleLabel);

  // Description
  auto *descLabel = new QLabel("Tests single-core, multi-core, integer, "
                               "floating-point, and memory performance.");
  descLabel->setStyleSheet("color: #9CDCFE;");
  mainLayout->addWidget(descLabel);

  // Start button
  m_startButton = new QPushButton("Start CPU Benchmark");
  m_startButton->setMinimumHeight(50);
  m_startButton->setStyleSheet("QPushButton {"
                               "  background-color: #569CD6;"
                               "  color: white;"
                               "  border: none;"
                               "  border-radius: 5px;"
                               "  font-size: 16px;"
                               "  font-weight: bold;"
                               "}"
                               "QPushButton:hover {"
                               "  background-color: #4080BF;"
                               "}"
                               "QPushButton:pressed {"
                               "  background-color: #366999;"
                               "}"
                               "QPushButton:disabled {"
                               "  background-color: #3F3F46;"
                               "  color: #808080;"
                               "}");
  connect(m_startButton, &QPushButton::clicked, this,
          &CPUBenchmarkWidget::onStartBenchmark);
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
                               "  background-color: #569CD6;"
                               "  border-radius: 4px;"
                               "}");
  mainLayout->addWidget(m_progressBar);

  // Status label
  m_statusLabel = new QLabel("Ready to start benchmark");
  m_statusLabel->setStyleSheet("color: #9CDCFE; font-size: 13px;");
  mainLayout->addWidget(m_statusLabel);

  // Results group
  m_resultsGroup = new QGroupBox("Benchmark Results");
  m_resultsGroup->setStyleSheet("QGroupBox {"
                                "  color: #CCCCCC;"
                                "  border: 1px solid #3F3F46;"
                                "  border-radius: 5px;"
                                "  margin-top: 10px;"
                                "  padding-top: 10px;"
                                "  font-size: 14px;"
                                "  font-weight: bold;"
                                "}"
                                "QGroupBox::title {"
                                "  subcontrol-origin: margin;"
                                "  left: 10px;"
                                "  padding: 0 5px 0 5px;"
                                "}");

  auto *resultsLayout = new QVBoxLayout();
  resultsLayout->setSpacing(10);

  // CPU name
  m_cpuNameLabel = new QLabel("CPU: -");
  m_cpuNameLabel->setStyleSheet("color: #DCDCAA; font-size: 12px;");
  resultsLayout->addWidget(m_cpuNameLabel);

  // Separator
  auto *separator1 = new QFrame();
  separator1->setFrameShape(QFrame::HLine);
  separator1->setStyleSheet("background-color: #3F3F46;");
  resultsLayout->addWidget(separator1);

  // Scores
  m_singleCoreScoreLabel = new QLabel("Single-Core Score: -");
  m_singleCoreScoreLabel->setStyleSheet("color: #CCCCCC; font-size: 14px;");
  resultsLayout->addWidget(m_singleCoreScoreLabel);

  m_multiCoreScoreLabel = new QLabel("Multi-Core Score: -");
  m_multiCoreScoreLabel->setStyleSheet("color: #CCCCCC; font-size: 14px;");
  resultsLayout->addWidget(m_multiCoreScoreLabel);

  auto *separator2 = new QFrame();
  separator2->setFrameShape(QFrame::HLine);
  separator2->setStyleSheet("background-color: #3F3F46;");
  resultsLayout->addWidget(separator2);

  m_integerScoreLabel = new QLabel("Integer Performance: -");
  m_integerScoreLabel->setStyleSheet("color: #9CDCFE; font-size: 12px;");
  resultsLayout->addWidget(m_integerScoreLabel);

  m_floatingPointScoreLabel = new QLabel("Floating-Point Performance: -");
  m_floatingPointScoreLabel->setStyleSheet("color: #9CDCFE; font-size: 12px;");
  resultsLayout->addWidget(m_floatingPointScoreLabel);

  m_memoryBandwidthLabel = new QLabel("Memory Bandwidth: -");
  m_memoryBandwidthLabel->setStyleSheet("color: #9CDCFE; font-size: 12px;");
  resultsLayout->addWidget(m_memoryBandwidthLabel);

  auto *separator3 = new QFrame();
  separator3->setFrameShape(QFrame::HLine);
  separator3->setStyleSheet("background-color: #3F3F46;");
  resultsLayout->addWidget(separator3);

  m_durationLabel = new QLabel("Duration: -");
  m_durationLabel->setStyleSheet(
      "color: #808080; font-size: 11px; font-style: italic;");
  resultsLayout->addWidget(m_durationLabel);

  m_resultsGroup->setLayout(resultsLayout);
  mainLayout->addWidget(m_resultsGroup);

  mainLayout->addStretch();
}

void CPUBenchmarkWidget::onStartBenchmark() {
  if (m_isRunning)
    return;

  m_isRunning = true;
  m_startButton->setEnabled(false);
  m_progressBar->setValue(0);
  m_statusLabel->setText("Initializing benchmark...");

  // Reset results
  m_singleCoreScoreLabel->setText("Single-Core Score: -");
  m_multiCoreScoreLabel->setText("Multi-Core Score: -");
  m_integerScoreLabel->setText("Integer Performance: -");
  m_floatingPointScoreLabel->setText("Floating-Point Performance: -");
  m_memoryBandwidthLabel->setText("Memory Bandwidth: -");
  m_durationLabel->setText("Duration: -");

  // Start benchmark in worker thread
  QMetaObject::invokeMethod(m_worker, "runBenchmark", Qt::QueuedConnection);
}

void CPUBenchmarkWidget::onBenchmarkProgress(int percent,
                                             const QString &status) {
  m_progressBar->setValue(percent);
  m_statusLabel->setText(status);
}

void CPUBenchmarkWidget::onBenchmarkComplete(const CPUBenchmarkResult &result) {
  m_isRunning = false;
  m_startButton->setEnabled(true);
  m_progressBar->setValue(100);
  m_statusLabel->setText("Benchmark Complete!");

  updateResultsDisplay(result);
}

void CPUBenchmarkWidget::updateResultsDisplay(
    const CPUBenchmarkResult &result) {
  // CPU name
  m_cpuNameLabel->setText(QString("CPU: %1 (%2 cores, %3 threads)")
                              .arg(QString::fromStdString(result.cpuName))
                              .arg(result.coreCount)
                              .arg(result.threadCount));

  // Main scores with highlighting
  m_singleCoreScoreLabel->setText(
      QString("<span style='font-size: 16px; font-weight: bold; color: "
              "#569CD6;'>Single-Core Score: %1</span>")
          .arg(result.singleCoreScore));

  m_multiCoreScoreLabel->setText(
      QString("<span style='font-size: 16px; font-weight: bold; color: "
              "#569CD6;'>Multi-Core Score: %1</span>")
          .arg(result.multiCoreScore));

  // Detailed metrics
  m_integerScoreLabel->setText(QString("Integer Performance: %1 pts")
                                   .arg(static_cast<int>(result.integerScore)));

  m_floatingPointScoreLabel->setText(
      QString("Floating-Point Performance: %1 pts")
          .arg(static_cast<int>(result.floatingPointScore)));

  m_memoryBandwidthLabel->setText(
      QString("Memory Bandwidth: %1 MB/s")
          .arg(static_cast<int>(result.memoryBandwidthMBps)));

  // Duration
  m_durationLabel->setText(
      QString("Duration: %1 seconds").arg(result.durationSeconds, 0, 'f', 2));
}
