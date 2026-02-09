/*
 * Sensor Monitor Implementation
 * User-mode interface to OmniSensor kernel driver
 * License: MIT
 */

#include "sensor_monitor.h"

#include <Psapi.h>
#include <Wbemidl.h>
#include <Windows.h>
#include <comdef.h>
#include <intrin.h>
#include <string>
#include <winioctl.h>

// IOCTL definitions (must match driver)
#define FILE_DEVICE_OMNISENSOR 0x8000
#define IOCTL_READ_MSR                                                         \
  CTL_CODE(FILE_DEVICE_OMNISENSOR, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WRITE_MSR                                                        \
  CTL_CODE(FILE_DEVICE_OMNISENSOR, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

// MSR addresses
#define MSR_RAPL_POWER_UNIT 0x606
#define MSR_PKG_ENERGY_STATUS 0x611
#define MSR_PACKAGE_THERM_STATUS 0x1B1
#define MSR_TEMPERATURE_TARGET 0x1A2
#define MSR_IA32_THERM_STATUS 0x19C
#define AMD_MSR_CORE_ENERGY 0xC001029A
#define AMD_MSR_PKG_ENERGY 0xC001029B

// Request structure for driver communication
#pragma pack(push, 1)
typedef struct _MSR_REQUEST_LOCAL {
  ULONG Register;  // MSR register address
  ULONG CoreIndex; // CPU core to read from (0-based)
  ULONGLONG Value; // Value read or to write
  LONG Status;     // NTSTATUS result
} MSR_REQUEST_LOCAL;
#pragma pack(pop)

SensorMonitor::SensorMonitor() { DetectCPUVendor(); }

SensorMonitor::~SensorMonitor() { Shutdown(); }

void SensorMonitor::DetectCPUVendor() {
  int cpuInfo[4] = {0};
  __cpuid(cpuInfo, 0);

  char vendor[13];
  memcpy(vendor, &cpuInfo[1], 4);
  memcpy(vendor + 4, &cpuInfo[3], 4);
  memcpy(vendor + 8, &cpuInfo[2], 4);
  vendor[12] = '\0';

  if (strcmp(vendor, "AuthenticAMD") == 0) {
    isAMD = true;
  } else if (strcmp(vendor, "GenuineIntel") == 0) {
    isIntel = true;
  }

  // Get core count
  SYSTEM_INFO sysInfo;
  GetSystemInfo(&sysInfo);
  coreCount = sysInfo.dwNumberOfProcessors;
}

bool SensorMonitor::Initialize() {
  // Try to open the driver
  hDriver = CreateFileW(L"\\\\.\\OmniSensor", GENERIC_READ | GENERIC_WRITE, 0,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

  if (hDriver != INVALID_HANDLE_VALUE) {
    driverLoaded = true;

    // Read TjMax for Intel CPUs
    if (isIntel) {
      uint64_t value;
      if (ReadMSR(MSR_TEMPERATURE_TARGET, 0, &value)) {
        tjMax = (float)((value >> 16) & 0xFF);
      }
    }

    // Read power units
    if (isIntel) {
      uint64_t value;
      if (ReadMSR(MSR_RAPL_POWER_UNIT, 0, &value)) {
        energyUnits = 1.0f / (float)(1 << (value & 0x1F));
      }
    }

    return true;
  }

  // Driver not available, use fallback methods
  driverLoaded = false;

  // Try to load NVML for GPU monitoring
  InitNVML();

  return true; // Still return true, we can use WMI fallback
}

void SensorMonitor::Shutdown() {
  if (hDriver != INVALID_HANDLE_VALUE) {
    CloseHandle(hDriver);
    hDriver = INVALID_HANDLE_VALUE;
  }
  driverLoaded = false;

  ShutdownNVML();
}

bool SensorMonitor::ReadMSR(uint32_t msrAddress, uint32_t coreIndex,
                            uint64_t *value) {
  if (!driverLoaded || hDriver == INVALID_HANDLE_VALUE) {
    return false;
  }

  MSR_REQUEST_LOCAL request = {0};
  request.Register = msrAddress;
  request.CoreIndex = coreIndex;

  DWORD bytesReturned;
  BOOL result =
      DeviceIoControl(hDriver, IOCTL_READ_MSR, &request, sizeof(request),
                      &request, sizeof(request), &bytesReturned, NULL);

  if (result && request.Status == 0) {
    *value = request.Value;
    return true;
  }
  return false;
}

bool SensorMonitor::WriteMSR(uint32_t msrAddress, uint32_t coreIndex,
                             uint64_t value) {
  if (!driverLoaded || hDriver == INVALID_HANDLE_VALUE) {
    return false;
  }

  MSR_REQUEST_LOCAL request = {0};
  request.Register = msrAddress;
  request.CoreIndex = coreIndex;
  request.Value = value;

  DWORD bytesReturned;
  return DeviceIoControl(hDriver, IOCTL_WRITE_MSR, &request, sizeof(request),
                         &request, sizeof(request), &bytesReturned,
                         NULL) != FALSE;
}

SensorMonitor::AllSensors SensorMonitor::ReadAll() {
  AllSensors sensors;
  sensors.driverLoaded = driverLoaded;

  sensors.cpu = ReadCPUSensors();
  sensors.gpu = ReadGPUSensors();
  sensors.ram = ReadRAMSensors();

  return sensors;
}

SensorMonitor::CPUSensors SensorMonitor::ReadCPUSensors() {
  if (driverLoaded) {
    CPUSensors sensors;
    sensors.coreCount = coreCount;
    sensors.tjMax = tjMax;

    if (isIntel) {
      // Intel: Read per-core temperatures via THERM_STATUS MSR
      for (uint32_t i = 0; i < coreCount && i < 64; i++) {
        sensors.coreTemps[i] = ReadIntelTemperature(i);
      }
      sensors.packageTemp =
          sensors.coreTemps[0]; // Use core 0 as package approx
      sensors.packagePowerW = ReadIntelPackagePower();
    } else if (isAMD) {
      // AMD: Read package temp via SMN
      sensors.packageTemp = ReadAMDTemperature();
      for (uint32_t i = 0; i < coreCount && i < 64; i++) {
        sensors.coreTemps[i] =
            sensors.packageTemp; // AMD reports Tdie, not per-core
      }
      sensors.packagePowerW = ReadAMDCorePower();
    }

    // Usage from PDH or GetSystemTimes
    FILETIME idleTime, kernelTime, userTime;
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
      static uint64_t prevIdle = 0, prevKernel = 0, prevUser = 0;

      uint64_t idle =
          ((uint64_t)idleTime.dwHighDateTime << 32) | idleTime.dwLowDateTime;
      uint64_t kernel = ((uint64_t)kernelTime.dwHighDateTime << 32) |
                        kernelTime.dwLowDateTime;
      uint64_t user =
          ((uint64_t)userTime.dwHighDateTime << 32) | userTime.dwLowDateTime;

      uint64_t totalDiff = (kernel - prevKernel) + (user - prevUser);
      uint64_t idleDiff = idle - prevIdle;

      if (totalDiff > 0) {
        sensors.usagePercent =
            100.0f * (1.0f - (float)idleDiff / (float)totalDiff);
      }

      prevIdle = idle;
      prevKernel = kernel;
      prevUser = user;
    }

    return sensors;
  }

  // Fallback to WMI
  return ReadCPUSensorsWMI();
}

float SensorMonitor::ReadIntelTemperature(uint32_t coreIndex) {
  uint64_t value;
  if (ReadMSR(MSR_IA32_THERM_STATUS, coreIndex, &value)) {
    // Bits 22:16 = Temperature reading
    uint32_t tempOffset = (value >> 16) & 0x7F;
    return tjMax - (float)tempOffset;
  }
  return 0.0f;
}

float SensorMonitor::ReadIntelPackagePower() {
  uint64_t value;
  if (ReadMSR(MSR_PKG_ENERGY_STATUS, 0, &value)) {
    uint32_t energy = value & 0xFFFFFFFF;

    if (lastEnergyReading > 0) {
      uint32_t delta = energy - (uint32_t)lastEnergyReading;
      float joules = delta * energyUnits;
      // Assuming ~1 second between readings
      lastEnergyReading = energy;
      return joules; // Watts = Joules/second
    }
    lastEnergyReading = energy;
  }
  return 0.0f;
}

float SensorMonitor::ReadAMDTemperature() {
  // AMD temperature is typically read via SMN (System Management Network)
  // which requires PCI config space access: write address to 0x60, read value
  // from 0x64 This is more complex and requires additional driver support For
  // now, return placeholder
  return 45.0f; // TODO: Implement SMN read
}

float SensorMonitor::ReadAMDCorePower() {
  uint64_t value;
  if (ReadMSR(AMD_MSR_CORE_ENERGY, 0, &value)) {
    // AMD uses fixed 15.3 uJ units
    if (lastEnergyReading > 0) {
      uint64_t delta = value - lastEnergyReading;
      float microjoules = delta * 15.3f;
      lastEnergyReading = value;
      return microjoules / 1000000.0f; // Convert to Watts
    }
    lastEnergyReading = value;
  }
  return 0.0f;
}

SensorMonitor::GPUSensors SensorMonitor::ReadGPUSensors() {
  // Try NVML first for NVIDIA GPUs
  if (nvmlLib != nullptr) {
    return ReadNVIDIASensors();
  }

  // Fallback to WMI
  return ReadGPUSensorsWMI();
}

SensorMonitor::RAMSensors SensorMonitor::ReadRAMSensors() {
  RAMSensors sensors;

  MEMORYSTATUSEX memStatus;
  memStatus.dwLength = sizeof(memStatus);
  if (GlobalMemoryStatusEx(&memStatus)) {
    sensors.totalGB =
        (float)memStatus.ullTotalPhys / (1024.0f * 1024.0f * 1024.0f);
    sensors.usedGB = (float)(memStatus.ullTotalPhys - memStatus.ullAvailPhys) /
                     (1024.0f * 1024.0f * 1024.0f);
    sensors.usagePercent = (float)memStatus.dwMemoryLoad;
  }

  // RAM speed would require WMI or registry access
  sensors.speedMHz = 5600.0f; // Placeholder - needs WMI query

  return sensors;
}

// NVML function typedefs
typedef int (*nvmlInit_t)();
typedef int (*nvmlShutdown_t)();
typedef int (*nvmlDeviceGetCount_t)(unsigned int *);
typedef int (*nvmlDeviceGetHandleByIndex_t)(unsigned int, void **);
typedef int (*nvmlDeviceGetTemperature_t)(void *, int, unsigned int *);
typedef int (*nvmlDeviceGetPowerUsage_t)(void *, unsigned int *);
typedef int (*nvmlDeviceGetClockInfo_t)(void *, int, unsigned int *);
typedef int (*nvmlDeviceGetUtilizationRates_t)(void *, void *);
typedef int (*nvmlDeviceGetMemoryInfo_t)(void *, void *);
typedef int (*nvmlDeviceGetFanSpeed_t)(void *, unsigned int *);
typedef int (*nvmlDeviceGetName_t)(void *, char *, unsigned int);

// NVML function pointers
static nvmlInit_t pNvmlInit = nullptr;
static nvmlShutdown_t pNvmlShutdown = nullptr;
static nvmlDeviceGetCount_t pNvmlDeviceGetCount = nullptr;
static nvmlDeviceGetHandleByIndex_t pNvmlDeviceGetHandleByIndex = nullptr;
static nvmlDeviceGetTemperature_t pNvmlDeviceGetTemperature = nullptr;
static nvmlDeviceGetPowerUsage_t pNvmlDeviceGetPowerUsage = nullptr;
static nvmlDeviceGetClockInfo_t pNvmlDeviceGetClockInfo = nullptr;
static nvmlDeviceGetUtilizationRates_t pNvmlDeviceGetUtilizationRates = nullptr;
static nvmlDeviceGetMemoryInfo_t pNvmlDeviceGetMemoryInfo = nullptr;
static nvmlDeviceGetFanSpeed_t pNvmlDeviceGetFanSpeed = nullptr;
static nvmlDeviceGetName_t pNvmlDeviceGetName = nullptr;

// NVML constants
#define NVML_TEMPERATURE_GPU 0
#define NVML_CLOCK_GRAPHICS 0
#define NVML_CLOCK_MEM 2

// NVML structs
struct nvmlUtilization_t {
  unsigned int gpu;
  unsigned int memory;
};

struct nvmlMemory_t {
  unsigned long long total;
  unsigned long long free;
  unsigned long long used;
};

bool SensorMonitor::InitNVML() {
  // Try to load nvml.dll from local directory first (self-contained), then
  // system paths
  wchar_t exePath[MAX_PATH];
  if (GetModuleFileNameW(NULL, exePath, MAX_PATH)) {
    wchar_t *lastSlash = wcsrchr(exePath, L'\\');
    if (lastSlash) {
      *lastSlash = L'\0';
      std::wstring localNvml = std::wstring(exePath) + L"\\nvml.dll";
      nvmlLib = LoadLibraryW(localNvml.c_str());
    }
  }

  // Fallback to system paths
  if (!nvmlLib) {
    nvmlLib = LoadLibraryA("nvml.dll");
  }

  if (!nvmlLib) {
    return false;
  }

  // Load function pointers
  pNvmlInit = (nvmlInit_t)GetProcAddress((HMODULE)nvmlLib, "nvmlInit_v2");
  if (!pNvmlInit) {
    pNvmlInit = (nvmlInit_t)GetProcAddress((HMODULE)nvmlLib, "nvmlInit");
  }

  pNvmlShutdown =
      (nvmlShutdown_t)GetProcAddress((HMODULE)nvmlLib, "nvmlShutdown");
  pNvmlDeviceGetCount = (nvmlDeviceGetCount_t)GetProcAddress(
      (HMODULE)nvmlLib, "nvmlDeviceGetCount_v2");
  pNvmlDeviceGetHandleByIndex = (nvmlDeviceGetHandleByIndex_t)GetProcAddress(
      (HMODULE)nvmlLib, "nvmlDeviceGetHandleByIndex_v2");
  pNvmlDeviceGetTemperature = (nvmlDeviceGetTemperature_t)GetProcAddress(
      (HMODULE)nvmlLib, "nvmlDeviceGetTemperature");
  pNvmlDeviceGetPowerUsage = (nvmlDeviceGetPowerUsage_t)GetProcAddress(
      (HMODULE)nvmlLib, "nvmlDeviceGetPowerUsage");
  pNvmlDeviceGetClockInfo = (nvmlDeviceGetClockInfo_t)GetProcAddress(
      (HMODULE)nvmlLib, "nvmlDeviceGetClockInfo");
  pNvmlDeviceGetUtilizationRates =
      (nvmlDeviceGetUtilizationRates_t)GetProcAddress(
          (HMODULE)nvmlLib, "nvmlDeviceGetUtilizationRates");
  pNvmlDeviceGetMemoryInfo = (nvmlDeviceGetMemoryInfo_t)GetProcAddress(
      (HMODULE)nvmlLib, "nvmlDeviceGetMemoryInfo");
  pNvmlDeviceGetFanSpeed = (nvmlDeviceGetFanSpeed_t)GetProcAddress(
      (HMODULE)nvmlLib, "nvmlDeviceGetFanSpeed");
  pNvmlDeviceGetName = (nvmlDeviceGetName_t)GetProcAddress((HMODULE)nvmlLib,
                                                           "nvmlDeviceGetName");

  if (!pNvmlInit) {
    FreeLibrary((HMODULE)nvmlLib);
    nvmlLib = nullptr;
    return false;
  }

  // Initialize NVML
  int result = pNvmlInit();
  if (result != 0) {
    FreeLibrary((HMODULE)nvmlLib);
    nvmlLib = nullptr;
    return false;
  }

  // Get first GPU handle
  unsigned int deviceCount = 0;
  if (pNvmlDeviceGetCount && pNvmlDeviceGetCount(&deviceCount) == 0 &&
      deviceCount > 0) {
    if (pNvmlDeviceGetHandleByIndex) {
      pNvmlDeviceGetHandleByIndex(0, &nvmlDevice);
    }
  }

  return nvmlDevice != nullptr;
}

void SensorMonitor::ShutdownNVML() {
  if (pNvmlShutdown && nvmlLib) {
    pNvmlShutdown();
  }
  if (nvmlLib) {
    FreeLibrary((HMODULE)nvmlLib);
    nvmlLib = nullptr;
  }
  nvmlDevice = nullptr;
}

SensorMonitor::GPUSensors SensorMonitor::ReadNVIDIASensors() {
  GPUSensors sensors;
  sensors.vendor = "NVIDIA";

  if (!nvmlLib || !nvmlDevice) {
    return sensors;
  }

  // Get GPU name
  if (pNvmlDeviceGetName) {
    char name[256] = {0};
    if (pNvmlDeviceGetName(nvmlDevice, name, sizeof(name)) == 0) {
      sensors.name = name;
    }
  }

  // Get temperature
  if (pNvmlDeviceGetTemperature) {
    unsigned int temp = 0;
    if (pNvmlDeviceGetTemperature(nvmlDevice, NVML_TEMPERATURE_GPU, &temp) ==
        0) {
      sensors.temperature = (float)temp;
    }
  }

  // Get power usage (returned in milliwatts)
  if (pNvmlDeviceGetPowerUsage) {
    unsigned int power = 0;
    if (pNvmlDeviceGetPowerUsage(nvmlDevice, &power) == 0) {
      sensors.powerW = (float)power / 1000.0f;
    }
  }

  // Get GPU clock
  if (pNvmlDeviceGetClockInfo) {
    unsigned int clock = 0;
    if (pNvmlDeviceGetClockInfo(nvmlDevice, NVML_CLOCK_GRAPHICS, &clock) == 0) {
      sensors.gpuClock = (float)clock;
    }
    if (pNvmlDeviceGetClockInfo(nvmlDevice, NVML_CLOCK_MEM, &clock) == 0) {
      sensors.memoryClock = (float)clock;
    }
  }

  // Get utilization
  if (pNvmlDeviceGetUtilizationRates) {
    nvmlUtilization_t util = {0};
    if (pNvmlDeviceGetUtilizationRates(nvmlDevice, &util) == 0) {
      sensors.usagePercent = (float)util.gpu;
    }
  }

  // Get memory info
  if (pNvmlDeviceGetMemoryInfo) {
    nvmlMemory_t mem = {0};
    if (pNvmlDeviceGetMemoryInfo(nvmlDevice, &mem) == 0) {
      sensors.memoryTotalMB = (float)(mem.total / (1024 * 1024));
      sensors.memoryUsedMB = (float)(mem.used / (1024 * 1024));
    }
  }

  // Get fan speed (percentage) - laptops may not support this
  if (pNvmlDeviceGetFanSpeed) {
    unsigned int fan = 0;
    int result = pNvmlDeviceGetFanSpeed(nvmlDevice, &fan);
    if (result == 0 && fan > 0) {
      sensors.fanSpeedPercent = (float)fan;
      // Estimate RPM (typical laptop max ~4500 RPM)
      sensors.fanSpeedRPM = (float)fan * 45.0f;
    } else {
      // For laptops: fan might be system-controlled or passive
      // Try to estimate based on GPU temperature (higher temp = higher fan)
      if (sensors.temperature > 0) {
        // Fan curve estimation: 0% at 40°C, 100% at 85°C
        float tempFactor = (sensors.temperature - 40.0f) / 45.0f;
        tempFactor = (tempFactor < 0) ? 0 : (tempFactor > 1) ? 1 : tempFactor;
        sensors.fanSpeedPercent = tempFactor * 100.0f;
        sensors.fanSpeedRPM = sensors.fanSpeedPercent * 45.0f; // ~4500 RPM max
      }
    }
  }

  return sensors;
}

// WMI fallback implementations
SensorMonitor::CPUSensors SensorMonitor::ReadCPUSensorsWMI() {
  CPUSensors sensors;
  sensors.coreCount = coreCount;

  // Get CPU usage from GetSystemTimes
  FILETIME idleTime, kernelTime, userTime;
  if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
    static uint64_t prevIdle = 0, prevKernel = 0, prevUser = 0;

    uint64_t idle =
        ((uint64_t)idleTime.dwHighDateTime << 32) | idleTime.dwLowDateTime;
    uint64_t kernel =
        ((uint64_t)kernelTime.dwHighDateTime << 32) | kernelTime.dwLowDateTime;
    uint64_t user =
        ((uint64_t)userTime.dwHighDateTime << 32) | userTime.dwLowDateTime;

    uint64_t totalDiff = (kernel - prevKernel) + (user - prevUser);
    uint64_t idleDiff = idle - prevIdle;

    if (totalDiff > 0) {
      sensors.usagePercent =
          100.0f * (1.0f - (float)idleDiff / (float)totalDiff);
    }

    prevIdle = idle;
    prevKernel = kernel;
    prevUser = user;
  }

  // Try to read CPU temperature from WMI thermal zones (requires admin)
  // This queries MSAcpi_ThermalZoneTemperature in root\WMI namespace
  static bool wmiTempInitialized = false;
  static IWbemLocator *pTempLocator = nullptr;
  static IWbemServices *pTempServices = nullptr;

  if (!wmiTempInitialized) {
    wmiTempInitialized = true;

    // Initialize COM if not already done
    HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE) {
      hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
                            IID_IWbemLocator, (LPVOID *)&pTempLocator);
      if (SUCCEEDED(hr) && pTempLocator) {
        // Connect to WMI root\WMI namespace for thermal data
        hr = pTempLocator->ConnectServer(_bstr_t(L"ROOT\\WMI"), NULL, NULL, 0,
                                         NULL, 0, 0, &pTempServices);
        if (SUCCEEDED(hr) && pTempServices) {
          CoSetProxyBlanket(pTempServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                            NULL, RPC_C_AUTHN_LEVEL_CALL,
                            RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
        }
      }
    }
  }

  // Query thermal zone temperature
  if (pTempServices) {
    IEnumWbemClassObject *pEnumerator = nullptr;
    HRESULT hr = pTempServices->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT CurrentTemperature FROM MSAcpi_ThermalZoneTemperature"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL,
        &pEnumerator);

    if (SUCCEEDED(hr) && pEnumerator) {
      IWbemClassObject *pObj = nullptr;
      ULONG returned = 0;

      if (pEnumerator->Next(WBEM_INFINITE, 1, &pObj, &returned) == S_OK &&
          pObj) {
        VARIANT var;
        if (SUCCEEDED(pObj->Get(L"CurrentTemperature", 0, &var, 0, 0))) {
          // Temperature is in tenths of Kelvin, convert to Celsius
          // Formula: Celsius = (Kelvin / 10) - 273.15
          if (var.vt == VT_I4 || var.vt == VT_UI4) {
            float kelvin = (float)var.ulVal / 10.0f;
            sensors.packageTemp = kelvin - 273.15f;
          }
          VariantClear(&var);
        }
        pObj->Release();
      }
      pEnumerator->Release();
    }
  }

  // Estimate CPU power based on TDP and usage
  // Typical AMD Ryzen 7840HS TDP is 54W, idle ~5W
  float baseTDP = 54.0f;  // AMD Ryzen 7840HS base TDP
  float idlePower = 5.0f; // Typical idle power
  sensors.packagePowerW =
      idlePower + (baseTDP - idlePower) * (sensors.usagePercent / 100.0f);

  return sensors;
}

SensorMonitor::GPUSensors SensorMonitor::ReadGPUSensorsWMI() {
  GPUSensors sensors;
  // WMI-based GPU detection is limited
  // Real implementation would query Win32_VideoController
  sensors.name = "GPU";
  return sensors;
}

// Static driver management functions
bool SensorMonitor::InstallDriver(const std::wstring &driverPath) {
  SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
  if (!schSCManager)
    return false;

  SC_HANDLE schService = CreateServiceW(
      schSCManager, L"OmniSensor", L"OmniSensor Hardware Monitor",
      SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START,
      SERVICE_ERROR_NORMAL, driverPath.c_str(), NULL, NULL, NULL, NULL, NULL);

  if (!schService) {
    // Service might already exist
    schService = OpenServiceW(schSCManager, L"OmniSensor", SERVICE_ALL_ACCESS);
  }

  bool result = schService != NULL;

  if (schService)
    CloseServiceHandle(schService);
  CloseServiceHandle(schSCManager);

  return result;
}

bool SensorMonitor::StartDriver() {
  SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
  if (!schSCManager)
    return false;

  SC_HANDLE schService =
      OpenServiceW(schSCManager, L"OmniSensor", SERVICE_START);
  if (!schService) {
    CloseServiceHandle(schSCManager);
    return false;
  }

  BOOL result = StartServiceW(schService, 0, NULL);

  CloseServiceHandle(schService);
  CloseServiceHandle(schSCManager);

  return result || GetLastError() == ERROR_SERVICE_ALREADY_RUNNING;
}

bool SensorMonitor::StopDriver() {
  SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
  if (!schSCManager)
    return false;

  SC_HANDLE schService =
      OpenServiceW(schSCManager, L"OmniSensor", SERVICE_STOP);
  if (!schService) {
    CloseServiceHandle(schSCManager);
    return false;
  }

  SERVICE_STATUS status;
  BOOL result = ControlService(schService, SERVICE_CONTROL_STOP, &status);

  CloseServiceHandle(schService);
  CloseServiceHandle(schSCManager);

  return result != FALSE;
}

bool SensorMonitor::UninstallDriver() {
  StopDriver();

  SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
  if (!schSCManager)
    return false;

  SC_HANDLE schService = OpenServiceW(schSCManager, L"OmniSensor", DELETE);
  if (!schService) {
    CloseServiceHandle(schSCManager);
    return false;
  }

  BOOL result = DeleteService(schService);

  CloseServiceHandle(schService);
  CloseServiceHandle(schSCManager);

  return result != FALSE;
}
