/*
 * Sensor Monitor - User-mode interface to OmniSensor kernel driver
 * Provides HWiNFO64-style accurate sensor readings via MSR access
 * License: MIT
 */

#pragma once

#include <Windows.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// Forward declare driver types
struct MSR_REQUEST;
struct CPU_TEMP_RESULT;
struct CPU_POWER_RESULT;

class SensorMonitor {
public:
  // Sensor data structures
  struct CPUSensors {
    float packageTemp = 0.0f;  // Package/Tdie temperature °C
    float coreTemps[64] = {0}; // Per-core temps °C
    uint32_t coreCount = 0;
    float tjMax = 100.0f; // Max junction temp

    float frequency = 0.0f; // Current frequency MHz
    float voltage = 0.0f;   // Core voltage V

    float packagePowerW = 0.0f; // Package power consumption W
    float corePowerW = 0.0f;    // Core-only power W

    float usagePercent = 0.0f; // CPU utilization %
  };

  struct GPUSensors {
    float temperature = 0.0f;     // GPU temp °C
    float fanSpeedRPM = 0.0f;     // Fan speed
    float fanSpeedPercent = 0.0f; // Fan % of max

    float gpuClock = 0.0f;    // GPU clock MHz
    float memoryClock = 0.0f; // VRAM clock MHz

    float powerW = 0.0f;      // Power draw W
    float powerLimitW = 0.0f; // Power limit W

    float usagePercent = 0.0f;  // GPU utilization %
    float memoryUsedMB = 0.0f;  // VRAM used MB
    float memoryTotalMB = 0.0f; // VRAM total MB

    std::string name;
    std::string vendor; // NVIDIA, AMD, Intel
  };

  struct RAMSensors {
    float totalGB = 0.0f;
    float usedGB = 0.0f;
    float usagePercent = 0.0f;
    float speedMHz = 0.0f;
  };

  struct AllSensors {
    CPUSensors cpu;
    GPUSensors gpu;
    RAMSensors ram;
    bool driverLoaded = false;
    std::string lastError;
  };

private:
  HANDLE hDriver = INVALID_HANDLE_VALUE;
  bool driverLoaded = false;
  bool isAMD = false;
  bool isIntel = false;
  uint32_t coreCount = 0;
  float tjMax = 100.0f;
  float energyUnits = 1.0f;
  uint64_t lastEnergyReading = 0;

  // NVIDIA NVML function pointers (loaded dynamically)
  void *nvmlLib = nullptr;
  void *nvmlDevice = nullptr;

public:
  SensorMonitor();
  ~SensorMonitor();

  // Initialize driver connection
  bool Initialize();
  void Shutdown();

  // Check if driver is available
  bool IsDriverLoaded() const { return driverLoaded; }

  // Read all sensors
  AllSensors ReadAll();

  // Read individual sensor categories
  CPUSensors ReadCPUSensors();
  GPUSensors ReadGPUSensors();
  RAMSensors ReadRAMSensors();

  // Load/unload driver (requires admin)
  static bool InstallDriver(const std::wstring &driverPath);
  static bool UninstallDriver();
  static bool StartDriver();
  static bool StopDriver();

  // NVIDIA GPU via NVML (public for dashboard access)
  bool InitNVML();
  void ShutdownNVML();
  GPUSensors ReadNVIDIASensors();

  // CPU sensors via WMI (public for dashboard access)
  CPUSensors ReadCPUSensorsWMI();

private:
  // Driver communication
  bool ReadMSR(uint32_t msrAddress, uint32_t coreIndex, uint64_t *value);
  bool WriteMSR(uint32_t msrAddress, uint32_t coreIndex, uint64_t value);

  // CPU vendor detection
  void DetectCPUVendor();

  // AMD-specific readings
  float ReadAMDTemperature();
  float ReadAMDCorePower();

  // Intel-specific readings
  float ReadIntelTemperature(uint32_t coreIndex);
  float ReadIntelPackagePower();

  // Fallback: WMI-based sensors
  GPUSensors ReadGPUSensorsWMI();
};
