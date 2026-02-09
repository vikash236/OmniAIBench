/*
 * Hardware Detector - WMI-based hardware detection
 * Reads CPU, GPU, RAM info from Windows Management Instrumentation
 * License: MIT
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>


class HardwareDetector {
public:
  // CPU Information
  struct CPUInfo {
    std::string name;   // e.g., "AMD Ryzen 7 7840HS"
    std::string vendor; // e.g., "AuthenticAMD" or "GenuineIntel"
    int physicalCores = 0;
    int logicalCores = 0;
    int baseClockMHz = 0;
    int maxClockMHz = 0;
    int l2CacheKB = 0;
    int l3CacheMB = 0;
    bool hasAVX2 = false;
    bool hasAVX512 = false;
    bool isAMDRyzenAI = false;   // Ryzen AI NPU present
    bool isIntelAIBoost = false; // Intel AI Boost present
  };

  // GPU Information
  struct GPUInfo {
    std::string name;   // e.g., "NVIDIA GeForce RTX 3050"
    std::string vendor; // NVIDIA, AMD, Intel
    std::string driverVersion;
    uint64_t vramBytes = 0;      // Dedicated VRAM
    uint64_t sharedMemBytes = 0; // Shared system memory
    int adapterIndex = 0;        // For multi-GPU systems
  };

  // RAM Information
  struct RAMInfo {
    uint64_t totalBytes = 0;
    uint64_t availableBytes = 0;
    int speedMHz = 0;
    int moduleCount = 0;
    std::string type; // e.g., "DDR5", "DDR4"
  };

  // Storage Information
  struct StorageInfo {
    std::string name;
    std::string type; // "SSD" or "HDD"
    uint64_t sizeBytes = 0;
    std::string interfaceType; // "NVMe", "SATA"
  };

  // System summary
  struct SystemInfo {
    CPUInfo cpu;
    std::vector<GPUInfo> gpus;
    RAMInfo ram;
    std::vector<StorageInfo> storage;
    std::string osVersion;
  };

public:
  HardwareDetector();
  ~HardwareDetector();

  // Initialize WMI connection
  bool Initialize();
  void Shutdown();

  // Detect all hardware
  SystemInfo DetectAll();

  // Individual detection methods
  CPUInfo DetectCPU();
  std::vector<GPUInfo> DetectGPUs();
  RAMInfo DetectRAM();
  std::vector<StorageInfo> DetectStorage();
  std::string DetectOSVersion();

private:
  bool wmiInitialized = false;
  void *wbemLocator = nullptr;  // IWbemLocator*
  void *wbemServices = nullptr; // IWbemServices*

  // CPUID-based detection
  void DetectCPUFromCPUID(CPUInfo &info);
  void DetectCPUFeatures(CPUInfo &info);

  // WMI query helpers
  std::string QueryWMIString(const wchar_t *wqlQuery, const wchar_t *property);
  uint64_t QueryWMINumber(const wchar_t *wqlQuery, const wchar_t *property);
  std::vector<std::pair<std::string, uint64_t>>
  QueryWMIMultiple(const wchar_t *wqlQuery, const wchar_t *stringProp,
                   const wchar_t *numProp);
};
