#pragma once
#include <atomic>
#include <chrono>
#include <functional>
#include <string>


struct CPUBenchmarkResult {
  int singleCoreScore;
  int multiCoreScore;
  double integerScore;
  double floatingPointScore;
  double memoryBandwidthMBps;
  double durationSeconds;
  std::string cpuName;
  int coreCount;
  int threadCount;
};

class CPUBenchmark {
public:
  using ProgressCallback = std::function<void(int, const std::string &)>;

  CPUBenchmark();
  ~CPUBenchmark();

  // Run full benchmark suite
  CPUBenchmarkResult run(ProgressCallback callback = nullptr);

  // Individual tests
  double runSingleCoreTest();
  double runMultiCoreTest();
  double runIntegerTest();
  double runFloatingPointTest();
  double runMemoryBandwidthTest();

  // Control
  void cancel();
  bool isRunning() const;

private:
  std::atomic<bool> m_running;
  std::atomic<bool> m_cancelled;

  // Test implementations
  uint64_t primeSieveTest(int limit);
  void matrixMultiplyTest(int size);
  void avx2FloatTest(size_t iterations);
  void memcpyBandwidthTest(size_t bufferSize, int iterations);

  // Score calculation (baseline: Intel i7-10700K @ 1000 points)
  int calculateScore(double timeSeconds, double baselineTimeSeconds);

  // Hardware info
  std::string getCPUName();
  int getCoreCount();
  int getThreadCount();
};
