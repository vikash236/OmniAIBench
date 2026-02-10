/*
 * GPU Benchmark Widget - Graphics & Compute performance testing
 * License: MIT
 */

#pragma once

#include "core/sensor_monitor.h"
#include <QGroupBox>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <atomic>
#include <chrono>
#include <memory>
#include <random>
#include <vector>

struct GPUBenchmarkResult {
  int computeScore = 0;
  int memoryScore = 0;
  int overallScore = 0;
  double computeGFLOPS = 0;
  double memoryBandwidthGBps = 0;
  double durationSeconds = 0;
  std::string gpuName;
};

class GPUBenchmarkWorker : public QObject {
  Q_OBJECT

public:
  GPUBenchmarkWorker() = default;

public slots:
  void runBenchmark() {
    std::atomic<bool> cancelled(false);
    GPUBenchmarkResult result;
    auto startTime = std::chrono::high_resolution_clock::now();

    emit progressUpdate(5, "Detecting GPU...");

    // Phase 1: Matrix operations (compute benchmark)
    emit progressUpdate(10, "Running compute benchmark (matrix operations)...");
    double computeOps = 0;
    {
      const int matSize = 512;
      std::vector<float> matA(matSize * matSize);
      std::vector<float> matB(matSize * matSize);
      std::vector<float> matC(matSize * matSize, 0);

      // Fill with random data
      std::mt19937 rng(42);
      std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
      for (int i = 0; i < matSize * matSize; ++i) {
        matA[i] = dist(rng);
        matB[i] = dist(rng);
      }

      auto compStart = std::chrono::high_resolution_clock::now();

      // Blocked matrix multiply (cache-friendly)
      const int blockSize = 64;
      for (int bi = 0; bi < matSize; bi += blockSize) {
        for (int bj = 0; bj < matSize; bj += blockSize) {
          for (int bk = 0; bk < matSize; bk += blockSize) {
            int iEnd = (std::min)(bi + blockSize, matSize);
            int jEnd = (std::min)(bj + blockSize, matSize);
            int kEnd = (std::min)(bk + blockSize, matSize);
            for (int i = bi; i < iEnd; ++i) {
              for (int k = bk; k < kEnd; ++k) {
                float aik = matA[i * matSize + k];
                for (int j = bj; j < jEnd; ++j) {
                  matC[i * matSize + j] += aik * matB[k * matSize + j];
                }
              }
            }
          }
        }
        int progress = 10 + (int)((float)bi / matSize * 40);
        emit progressUpdate(progress, "Compute benchmark running...");
      }

      auto compEnd = std::chrono::high_resolution_clock::now();
      double compSec =
          std::chrono::duration<double>(compEnd - compStart).count();

      // 2 * N^3 FLOPs for matrix multiply
      computeOps = 2.0 * matSize * matSize * matSize;
      result.computeGFLOPS = (computeOps / compSec) / 1e9;
      result.computeScore = (int)(result.computeGFLOPS * 100);
    }
    emit progressUpdate(50,
                        "Compute benchmark complete. Starting memory test...");

    // Phase 2: Memory bandwidth test
    {
      const size_t bufferSize = 128 * 1024 * 1024; // 128 MB
      std::vector<float> buffer(bufferSize / sizeof(float));
      std::vector<float> dest(bufferSize / sizeof(float));

      // Fill buffer
      for (size_t i = 0; i < buffer.size(); ++i) {
        buffer[i] = (float)i * 0.001f;
      }

      auto memStart = std::chrono::high_resolution_clock::now();
      int passes = 5;
      for (int p = 0; p < passes; ++p) {
        // Sequential copy
        std::memcpy(dest.data(), buffer.data(), bufferSize);
        // Read-modify-write
        for (size_t i = 0; i < buffer.size(); ++i) {
          dest[i] = buffer[i] * 1.5f + 0.5f;
        }
        int progress = 50 + (int)((float)(p + 1) / passes * 40);
        emit progressUpdate(progress, "Memory bandwidth test running...");
      }
      auto memEnd = std::chrono::high_resolution_clock::now();
      double memSec = std::chrono::duration<double>(memEnd - memStart).count();

      double totalBytes =
          (double)bufferSize * passes * 3.0; // read + write + RMW
      result.memoryBandwidthGBps = (totalBytes / memSec) / 1e9;
      result.memoryScore = (int)(result.memoryBandwidthGBps * 50);
    }

    emit progressUpdate(95, "Calculating final score...");

    result.overallScore =
        (int)(result.computeScore * 0.6 + result.memoryScore * 0.4);

    auto endTime = std::chrono::high_resolution_clock::now();
    result.durationSeconds =
        std::chrono::duration<double>(endTime - startTime).count();

    emit progressUpdate(100, "GPU Benchmark Complete!");
    emit benchmarkComplete(result);
  }

signals:
  void progressUpdate(int percent, const QString &status);
  void benchmarkComplete(const GPUBenchmarkResult &result);
};

class GPUBenchmarkWidget : public QWidget {
  Q_OBJECT

public:
  explicit GPUBenchmarkWidget(QWidget *parent = nullptr);
  ~GPUBenchmarkWidget();

private:
  void setupUI();

  QPushButton *m_startButton = nullptr;
  QProgressBar *m_progressBar = nullptr;
  QLabel *m_statusLabel = nullptr;
  QGroupBox *m_resultsGroup = nullptr;
  QLabel *m_gpuNameLabel = nullptr;
  QLabel *m_computeScoreLabel = nullptr;
  QLabel *m_memoryScoreLabel = nullptr;
  QLabel *m_overallScoreLabel = nullptr;
  QLabel *m_computeGflopsLabel = nullptr;
  QLabel *m_memBandwidthLabel = nullptr;
  QLabel *m_durationLabel = nullptr;

  // Live sensor display
  QLabel *m_liveGpuTemp = nullptr;
  QLabel *m_liveGpuClock = nullptr;
  QLabel *m_liveGpuPower = nullptr;
  QTimer *m_sensorTimer = nullptr;
  std::unique_ptr<SensorMonitor> m_sensorMonitor;
  bool m_nvmlAvailable = false;

  GPUBenchmarkWorker *m_worker = nullptr;
  QThread *m_benchmarkThread = nullptr;
  bool m_isRunning = false;

  void updateResultsDisplay(const GPUBenchmarkResult &result);

private slots:
  void onStartBenchmark();
  void onBenchmarkProgress(int percent, const QString &status);
  void onBenchmarkComplete(const GPUBenchmarkResult &result);
  void updateLiveSensors();
};
