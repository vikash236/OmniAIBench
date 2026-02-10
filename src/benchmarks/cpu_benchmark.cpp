#include "cpu_benchmark.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <immintrin.h>
#include <intrin.h>
#include <thread>
#include <vector>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

CPUBenchmark::CPUBenchmark() : m_running(false), m_cancelled(false) {}

CPUBenchmark::~CPUBenchmark() { cancel(); }

std::string CPUBenchmark::getCPUName() {
  int cpuInfo[4] = {0};
  char brand[64] = {0};

  __cpuid(cpuInfo, 0x80000000);
  unsigned int extCount = cpuInfo[0];

  if (extCount >= 0x80000004) {
    __cpuid((int *)(brand + 0), 0x80000002);
    __cpuid((int *)(brand + 16), 0x80000003);
    __cpuid((int *)(brand + 32), 0x80000004);
  }

  std::string result(brand);
  // Trim leading/trailing whitespace
  size_t start = result.find_first_not_of(" \t");
  size_t end = result.find_last_not_of(" \t");
  if (start != std::string::npos && end != std::string::npos) {
    result = result.substr(start, end - start + 1);
  }

  return result;
}

int CPUBenchmark::getCoreCount() {
  SYSTEM_INFO sysInfo;
  GetSystemInfo(&sysInfo);
  return sysInfo.dwNumberOfProcessors / 2; // Approximate physical cores
}

int CPUBenchmark::getThreadCount() {
  return std::thread::hardware_concurrency();
}

CPUBenchmarkResult CPUBenchmark::run(ProgressCallback callback) {
  m_running = true;
  m_cancelled = false;

  CPUBenchmarkResult result = {};
  result.cpuName = getCPUName();
  result.coreCount = getCoreCount();
  result.threadCount = getThreadCount();

  auto startTime = std::chrono::high_resolution_clock::now();

  // Test 1: Single-core performance (20%)
  if (callback)
    callback(0, "Running Single-Core Test...");
  if (!m_cancelled) {
    result.singleCoreScore = static_cast<int>(runSingleCoreTest());
  }

  // Test 2: Multi-core performance (30%)
  if (callback)
    callback(20, "Running Multi-Core Test...");
  if (!m_cancelled) {
    result.multiCoreScore = static_cast<int>(runMultiCoreTest());
  }

  // Test 3: Integer arithmetic (20%)
  if (callback)
    callback(50, "Running Integer Test...");
  if (!m_cancelled) {
    result.integerScore = runIntegerTest();
  }

  // Test 4: Floating-point performance (20%)
  if (callback)
    callback(70, "Running Floating-Point Test...");
  if (!m_cancelled) {
    result.floatingPointScore = runFloatingPointTest();
  }

  // Test 5: Memory bandwidth (10%)
  if (callback)
    callback(90, "Running Memory Bandwidth Test...");
  if (!m_cancelled) {
    result.memoryBandwidthMBps = runMemoryBandwidthTest();
  }

  auto endTime = std::chrono::high_resolution_clock::now();
  result.durationSeconds =
      std::chrono::duration<double>(endTime - startTime).count();

  if (callback)
    callback(100, "Benchmark Complete");
  m_running = false;

  return result;
}

// Prime Sieve (Sieve of Eratosthenes) - Integer performance test
uint64_t CPUBenchmark::primeSieveTest(int limit) {
  std::vector<bool> isPrime(limit + 1, true);
  isPrime[0] = isPrime[1] = false;

  for (int i = 2; i * i <= limit; ++i) {
    if (m_cancelled)
      return 0;
    if (isPrime[i]) {
      for (int j = i * i; j <= limit; j += i) {
        isPrime[j] = false;
      }
    }
  }

  uint64_t count = 0;
  for (bool prime : isPrime) {
    if (prime)
      count++;
  }

  return count;
}

double CPUBenchmark::runSingleCoreTest() {
  auto start = std::chrono::high_resolution_clock::now();

  // Run prime sieve test on a single core
  const int limit = 10000000; // 10 million
  uint64_t primeCount = primeSieveTest(limit);

  auto end = std::chrono::high_resolution_clock::now();
  double elapsed = std::chrono::duration<double>(end - start).count();

  // Baseline: i7-10700K completes in ~0.5 seconds = 1000 points
  // Score = (baseline_time / actual_time) * 1000
  double baselineTime = 0.5;
  return calculateScore(elapsed, baselineTime);
}

void CPUBenchmark::matrixMultiplyTest(int size) {
  std::vector<float> A(size * size);
  std::vector<float> B(size * size);
  std::vector<float> C(size * size);

  // Initialize matrices
  for (int i = 0; i < size * size; ++i) {
    A[i] = static_cast<float>(i % 100) / 10.0f;
    B[i] = static_cast<float>(i % 100) / 10.0f;
  }

  // Matrix multiplication
  for (int i = 0; i < size; ++i) {
    if (m_cancelled)
      return;
    for (int j = 0; j < size; ++j) {
      float sum = 0.0f;
      for (int k = 0; k < size; ++k) {
        sum += A[i * size + k] * B[k * size + j];
      }
      C[i * size + j] = sum;
    }
  }
}

double CPUBenchmark::runMultiCoreTest() {
  auto start = std::chrono::high_resolution_clock::now();

  const int matrixSize = 512;
  int numThreads = getThreadCount();
  std::vector<std::thread> threads;

  // Spawn threads for parallel matrix multiplication
  for (int t = 0; t < numThreads; ++t) {
    threads.emplace_back(
        [this, matrixSize]() { matrixMultiplyTest(matrixSize); });
  }

  // Wait for all threads
  for (auto &thread : threads) {
    thread.join();
  }

  auto end = std::chrono::high_resolution_clock::now();
  double elapsed = std::chrono::duration<double>(end - start).count();

  // Baseline: i7-10700K (8 cores/16 threads) completes in ~2.5 seconds = 1000
  // points
  double baselineTime = 2.5;
  return calculateScore(elapsed, baselineTime);
}

double CPUBenchmark::runIntegerTest() {
  auto start = std::chrono::high_resolution_clock::now();

  // Integer arithmetic heavy workload
  const size_t iterations = 100000000;
  volatile uint64_t result = 0;

  for (size_t i = 0; i < iterations; ++i) {
    if (m_cancelled)
      break;
    result += i * 17;
    result -= i / 3;
    result ^= i << 2;
  }

  auto end = std::chrono::high_resolution_clock::now();
  double elapsed = std::chrono::duration<double>(end - start).count();

  // Baseline: 1.0 second = 1000 points
  double baselineTime = 1.0;
  return calculateScore(elapsed, baselineTime);
}

void CPUBenchmark::avx2FloatTest(size_t iterations) {
  const size_t arraySize = 1024;
  alignas(32) float a[arraySize];
  alignas(32) float b[arraySize];
  alignas(32) float c[arraySize];

  // Initialize
  for (size_t i = 0; i < arraySize; ++i) {
    a[i] = static_cast<float>(i) * 0.1f;
    b[i] = static_cast<float>(i) * 0.2f;
  }

  // AVX2 vectorized operations
  for (size_t iter = 0; iter < iterations; ++iter) {
    if (m_cancelled)
      return;

    for (size_t i = 0; i < arraySize; i += 8) {
      __m256 va = _mm256_load_ps(&a[i]);
      __m256 vb = _mm256_load_ps(&b[i]);
      __m256 vc = _mm256_mul_ps(va, vb);
      vc = _mm256_add_ps(vc, va);
      vc = _mm256_sqrt_ps(vc);
      _mm256_store_ps(&c[i], vc);
    }
  }
}

double CPUBenchmark::runFloatingPointTest() {
  auto start = std::chrono::high_resolution_clock::now();

  // AVX2 floating-point operations
  const size_t iterations = 50000;
  avx2FloatTest(iterations);

  auto end = std::chrono::high_resolution_clock::now();
  double elapsed = std::chrono::duration<double>(end - start).count();

  // Baseline: 0.8 seconds = 1000 points
  double baselineTime = 0.8;
  return calculateScore(elapsed, baselineTime);
}

void CPUBenchmark::memcpyBandwidthTest(size_t bufferSize, int iterations) {
  std::vector<char> src(bufferSize);
  std::vector<char> dst(bufferSize);

  // Initialize source
  std::fill(src.begin(), src.end(), 0xAA);

  for (int i = 0; i < iterations; ++i) {
    if (m_cancelled)
      return;
    std::memcpy(dst.data(), src.data(), bufferSize);
  }
}

double CPUBenchmark::runMemoryBandwidthTest() {
  const size_t bufferSize = 256 * 1024 * 1024; // 256 MB
  const int iterations = 10;

  auto start = std::chrono::high_resolution_clock::now();
  memcpyBandwidthTest(bufferSize, iterations);
  auto end = std::chrono::high_resolution_clock::now();

  double elapsed = std::chrono::duration<double>(end - start).count();
  double totalBytes = static_cast<double>(bufferSize) * iterations;
  double bandwidthMBps = (totalBytes / (1024.0 * 1024.0)) / elapsed;

  return bandwidthMBps;
}

int CPUBenchmark::calculateScore(double timeSeconds,
                                 double baselineTimeSeconds) {
  if (timeSeconds <= 0.001)
    return 0; // Avoid division by zero

  // Score = (baseline_time / actual_time) * 1000
  // Faster execution = higher score
  double score = (baselineTimeSeconds / timeSeconds) * 1000.0;
  return static_cast<int>(std::max(0.0, score));
}

void CPUBenchmark::cancel() { m_cancelled = true; }

bool CPUBenchmark::isRunning() const { return m_running; }
