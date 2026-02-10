#pragma once
#include "../benchmarks/cpu_benchmark.h"
#include <QGroupBox>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>


class CPUBenchmarkWorker : public QObject {
  Q_OBJECT

public:
  CPUBenchmarkWorker() : m_benchmark(new CPUBenchmark()) {}
  ~CPUBenchmarkWorker() { delete m_benchmark; }

public slots:
  void runBenchmark() {
    auto callback = [this](int progress, const std::string &status) {
      emit progressUpdate(progress, QString::fromStdString(status));
    };

    CPUBenchmarkResult result = m_benchmark->run(callback);
    emit benchmarkComplete(result);
  }

  void cancelBenchmark() { m_benchmark->cancel(); }

signals:
  void progressUpdate(int progress, const QString &status);
  void benchmarkComplete(const CPUBenchmarkResult &result);

private:
  CPUBenchmark *m_benchmark;
};

class CPUBenchmarkWidget : public QWidget {
  Q_OBJECT

public:
  explicit CPUBenchmarkWidget(QWidget *parent = nullptr);
  ~CPUBenchmarkWidget();

private slots:
  void onStartBenchmark();
  void onBenchmarkProgress(int percent, const QString &status);
  void onBenchmarkComplete(const CPUBenchmarkResult &result);

private:
  void setupUI();
  void updateResultsDisplay(const CPUBenchmarkResult &result);

  QPushButton *m_startButton;
  QProgressBar *m_progressBar;
  QLabel *m_statusLabel;
  QGroupBox *m_resultsGroup;
  QLabel *m_singleCoreScoreLabel;
  QLabel *m_multiCoreScoreLabel;
  QLabel *m_integerScoreLabel;
  QLabel *m_floatingPointScoreLabel;
  QLabel *m_memoryBandwidthLabel;
  QLabel *m_cpuNameLabel;
  QLabel *m_durationLabel;

  CPUBenchmarkWorker *m_worker;
  QThread *m_benchmarkThread;
  bool m_isRunning;
};
