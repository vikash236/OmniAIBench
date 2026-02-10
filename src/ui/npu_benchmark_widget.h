/*
 * NPU Benchmark Widget - AI/Neural Processing Unit performance testing
 * License: MIT
 */

#pragma once

#include "core/hardware_detector.h"
#include <QGroupBox>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QThread>
#include <QVBoxLayout>
#include <QWidget>
#include <chrono>
#include <memory>
#include <random>
#include <vector>

struct NPUBenchmarkResult {
  int inferenceScore = 0;
  int throughputScore = 0;
  int overallScore = 0;
  double latencyMs = 0;
  double throughputOps = 0;
  double durationSeconds = 0;
  std::string npuType; // "AMD Ryzen AI", "Intel AI Boost", "None"
  bool npuDetected = false;
};

class NPUBenchmarkWorker : public QObject {
  Q_OBJECT

public:
  NPUBenchmarkWorker() = default;

public slots:
  void runBenchmark() {
    NPUBenchmarkResult result;
    auto startTime = std::chrono::high_resolution_clock::now();

    emit progressUpdate(5, "Initializing AI inference benchmark...");

    // Phase 1: Simulated neural network inference (matrix ops)
    emit progressUpdate(10, "Running inference benchmark (dense layers)...");
    {
      const int batchSize = 64;
      const int inputSize = 784; // MNIST-like
      const int hiddenSize = 512;
      const int outputSize = 10;

      std::mt19937 rng(42);
      std::uniform_real_distribution<float> dist(-0.1f, 0.1f);

      // Allocate weights
      std::vector<float> w1(inputSize * hiddenSize);
      std::vector<float> w2(hiddenSize * outputSize);
      std::vector<float> input(batchSize * inputSize);
      std::vector<float> hidden(batchSize * hiddenSize);
      std::vector<float> output(batchSize * outputSize);

      for (auto &v : w1)
        v = dist(rng);
      for (auto &v : w2)
        v = dist(rng);
      for (auto &v : input)
        v = dist(rng);

      auto infStart = std::chrono::high_resolution_clock::now();
      int totalInferences = 0;

      // Run inference passes
      for (int pass = 0; pass < 100; ++pass) {
        // Layer 1: input * w1 -> hidden (with ReLU)
        for (int b = 0; b < batchSize; ++b) {
          for (int h = 0; h < hiddenSize; ++h) {
            float sum = 0;
            for (int i = 0; i < inputSize; ++i) {
              sum += input[b * inputSize + i] * w1[i * hiddenSize + h];
            }
            hidden[b * hiddenSize + h] = sum > 0 ? sum : 0; // ReLU
          }
        }

        // Layer 2: hidden * w2 -> output (softmax skip for benchmark)
        for (int b = 0; b < batchSize; ++b) {
          for (int o = 0; o < outputSize; ++o) {
            float sum = 0;
            for (int h = 0; h < hiddenSize; ++h) {
              sum += hidden[b * hiddenSize + h] * w2[h * outputSize + o];
            }
            output[b * outputSize + o] = sum;
          }
        }

        totalInferences += batchSize;
        if (pass % 10 == 0) {
          int progress = 10 + (int)((float)pass / 100 * 50);
          emit progressUpdate(progress, "Inference benchmark running...");
        }
      }

      auto infEnd = std::chrono::high_resolution_clock::now();
      double infSec = std::chrono::duration<double>(infEnd - infStart).count();

      result.latencyMs = (infSec / totalInferences) * 1000.0;
      result.throughputOps = totalInferences / infSec;
      result.inferenceScore = (int)(result.throughputOps / 10);
    }
    emit progressUpdate(65, "Inference complete. Running throughput test...");

    // Phase 2: Throughput test (convolution-like operations)
    {
      const int imageSize = 224;
      const int channels = 3;
      const int filterSize = 3;
      const int numFilters = 16;

      std::mt19937 rng(123);
      std::uniform_real_distribution<float> dist(-0.5f, 0.5f);

      std::vector<float> image(imageSize * imageSize * channels);
      std::vector<float> filters(numFilters * filterSize * filterSize *
                                 channels);
      std::vector<float> convOutput((imageSize - filterSize + 1) *
                                    (imageSize - filterSize + 1) * numFilters);

      for (auto &v : image)
        v = dist(rng);
      for (auto &v : filters)
        v = dist(rng);

      auto convStart = std::chrono::high_resolution_clock::now();

      // Simple convolution
      int outSize = imageSize - filterSize + 1;
      for (int f = 0; f < numFilters; ++f) {
        for (int oy = 0; oy < outSize; ++oy) {
          for (int ox = 0; ox < outSize; ++ox) {
            float sum = 0;
            for (int c = 0; c < channels; ++c) {
              for (int fy = 0; fy < filterSize; ++fy) {
                for (int fx = 0; fx < filterSize; ++fx) {
                  int iy = oy + fy;
                  int ix = ox + fx;
                  sum += image[(iy * imageSize + ix) * channels + c] *
                         filters[((f * filterSize + fy) * filterSize + fx) *
                                     channels +
                                 c];
                }
              }
            }
            convOutput[(f * outSize + oy) * outSize + ox] =
                sum > 0 ? sum : 0; // ReLU
          }
        }
        int progress = 65 + (int)((float)(f + 1) / numFilters * 25);
        emit progressUpdate(progress, "Convolution throughput test...");
      }

      auto convEnd = std::chrono::high_resolution_clock::now();
      double convSec =
          std::chrono::duration<double>(convEnd - convStart).count();

      double convOps = 2.0 * numFilters * outSize * outSize * channels *
                       filterSize * filterSize;
      result.throughputScore = (int)((convOps / convSec) / 1e6);
    }

    emit progressUpdate(95, "Calculating final score...");

    result.overallScore =
        (int)(result.inferenceScore * 0.5 + result.throughputScore * 0.5);

    auto endTime = std::chrono::high_resolution_clock::now();
    result.durationSeconds =
        std::chrono::duration<double>(endTime - startTime).count();

    emit progressUpdate(100, "NPU Benchmark Complete!");
    emit benchmarkComplete(result);
  }

signals:
  void progressUpdate(int percent, const QString &status);
  void benchmarkComplete(const NPUBenchmarkResult &result);
};

class NPUBenchmarkWidget : public QWidget {
  Q_OBJECT

public:
  explicit NPUBenchmarkWidget(QWidget *parent = nullptr);
  ~NPUBenchmarkWidget();

private:
  void setupUI();
  void detectNPU();

  QPushButton *m_startButton = nullptr;
  QProgressBar *m_progressBar = nullptr;
  QLabel *m_statusLabel = nullptr;
  QLabel *m_npuDetectionLabel = nullptr;
  QGroupBox *m_resultsGroup = nullptr;
  QLabel *m_overallScoreLabel = nullptr;
  QLabel *m_inferenceScoreLabel = nullptr;
  QLabel *m_throughputScoreLabel = nullptr;
  QLabel *m_latencyLabel = nullptr;
  QLabel *m_throughputOpsLabel = nullptr;
  QLabel *m_durationLabel = nullptr;

  std::unique_ptr<HardwareDetector> m_hwDetector;
  NPUBenchmarkWorker *m_worker = nullptr;
  QThread *m_benchmarkThread = nullptr;
  bool m_isRunning = false;
  bool m_npuDetected = false;
  QString m_npuType;

  void updateResultsDisplay(const NPUBenchmarkResult &result);

private slots:
  void onStartBenchmark();
  void onBenchmarkProgress(int percent, const QString &status);
  void onBenchmarkComplete(const NPUBenchmarkResult &result);
};
