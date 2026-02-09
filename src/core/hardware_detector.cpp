/*
 * Hardware Detector Implementation - WMI-based hardware detection
 * License: MIT
 */

#include "hardware_detector.h"

#include <Wbemidl.h>
#include <Windows.h>
#include <algorithm>
#include <comdef.h>
#include <intrin.h>
#include <string>
#include <vector>

#pragma comment(lib, "wbemuuid.lib")

HardwareDetector::HardwareDetector() {}

HardwareDetector::~HardwareDetector() { Shutdown(); }

bool HardwareDetector::Initialize() {
  // Initialize COM
  HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
  if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
    return false;
  }

  // Set COM security
  hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
                            RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);

  // Create WMI locator
  IWbemLocator *locator = nullptr;
  hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
                        IID_IWbemLocator, (LPVOID *)&locator);

  if (FAILED(hr)) {
    return false;
  }
  wbemLocator = locator;

  // Connect to WMI namespace
  IWbemServices *services = nullptr;
  hr = locator->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0,
                              0, &services);

  if (FAILED(hr)) {
    locator->Release();
    wbemLocator = nullptr;
    return false;
  }
  wbemServices = services;

  // Set proxy security
  hr = CoSetProxyBlanket(services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                         RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE,
                         NULL, EOAC_NONE);

  wmiInitialized = true;
  return true;
}

void HardwareDetector::Shutdown() {
  if (wbemServices) {
    ((IWbemServices *)wbemServices)->Release();
    wbemServices = nullptr;
  }
  if (wbemLocator) {
    ((IWbemLocator *)wbemLocator)->Release();
    wbemLocator = nullptr;
  }
  wmiInitialized = false;
}

HardwareDetector::SystemInfo HardwareDetector::DetectAll() {
  SystemInfo info;
  info.cpu = DetectCPU();
  info.gpus = DetectGPUs();
  info.ram = DetectRAM();
  info.storage = DetectStorage();
  info.osVersion = DetectOSVersion();
  return info;
}

void HardwareDetector::DetectCPUFromCPUID(CPUInfo &info) {
  int cpuInfo[4] = {0};

  // Get vendor string
  __cpuid(cpuInfo, 0);
  char vendor[13];
  memcpy(vendor, &cpuInfo[1], 4);
  memcpy(vendor + 4, &cpuInfo[3], 4);
  memcpy(vendor + 8, &cpuInfo[2], 4);
  vendor[12] = '\0';
  info.vendor = vendor;

  // Get CPU brand string
  char brand[49] = {0};
  __cpuid(cpuInfo, 0x80000000);
  if ((unsigned int)cpuInfo[0] >= 0x80000004) {
    __cpuid(cpuInfo, 0x80000002);
    memcpy(brand, cpuInfo, 16);
    __cpuid(cpuInfo, 0x80000003);
    memcpy(brand + 16, cpuInfo, 16);
    __cpuid(cpuInfo, 0x80000004);
    memcpy(brand + 32, cpuInfo, 16);

    // Trim leading spaces
    char *p = brand;
    while (*p == ' ')
      p++;
    info.name = p;
  }
}

void HardwareDetector::DetectCPUFeatures(CPUInfo &info) {
  int cpuInfo[4];

  // Check for AVX2 (EAX=7, ECX=0, check EBX bit 5)
  __cpuidex(cpuInfo, 7, 0);
  info.hasAVX2 = (cpuInfo[1] & (1 << 5)) != 0;

  // Check for AVX-512 (EAX=7, ECX=0, check EBX bit 16)
  info.hasAVX512 = (cpuInfo[1] & (1 << 16)) != 0;

  // Detect AMD Ryzen AI (7040/7840/7940 series with XDNA NPU)
  if (info.name.find("7840") != std::string::npos ||
      info.name.find("7940") != std::string::npos ||
      info.name.find("7040") != std::string::npos ||
      info.name.find("8840") != std::string::npos ||
      info.name.find("8845") != std::string::npos) {
    info.isAMDRyzenAI = true;
  }

  // Detect Intel AI Boost (Meteor Lake with NPU)
  if (info.name.find("Ultra") != std::string::npos &&
      info.name.find("Core") != std::string::npos) {
    info.isIntelAIBoost = true;
  }
}

HardwareDetector::CPUInfo HardwareDetector::DetectCPU() {
  CPUInfo info;

  // Get name and vendor from CPUID
  DetectCPUFromCPUID(info);
  DetectCPUFeatures(info);

  // Get core counts from system
  SYSTEM_INFO sysInfo;
  GetSystemInfo(&sysInfo);
  info.logicalCores = sysInfo.dwNumberOfProcessors;

  // Get physical cores from WMI
  if (wmiInitialized) {
    IWbemServices *services = (IWbemServices *)wbemServices;
    IEnumWbemClassObject *enumerator = nullptr;

    HRESULT hr = services->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT NumberOfCores, MaxClockSpeed FROM Win32_Processor"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL,
        &enumerator);

    if (SUCCEEDED(hr)) {
      IWbemClassObject *obj = nullptr;
      ULONG returned = 0;

      if (enumerator->Next(WBEM_INFINITE, 1, &obj, &returned) == S_OK) {
        VARIANT var;

        if (SUCCEEDED(obj->Get(L"NumberOfCores", 0, &var, 0, 0))) {
          info.physicalCores = var.intVal;
          VariantClear(&var);
        }

        if (SUCCEEDED(obj->Get(L"MaxClockSpeed", 0, &var, 0, 0))) {
          info.maxClockMHz = var.intVal;
          VariantClear(&var);
        }

        obj->Release();
      }
      enumerator->Release();
    }
  }

  return info;
}

std::vector<HardwareDetector::GPUInfo> HardwareDetector::DetectGPUs() {
  std::vector<GPUInfo> gpus;

  if (!wmiInitialized)
    return gpus;

  IWbemServices *services = (IWbemServices *)wbemServices;
  IEnumWbemClassObject *enumerator = nullptr;

  HRESULT hr = services->ExecQuery(
      bstr_t("WQL"),
      bstr_t(
          "SELECT Name, AdapterRAM, DriverVersion FROM Win32_VideoController"),
      WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &enumerator);

  if (FAILED(hr))
    return gpus;

  IWbemClassObject *obj = nullptr;
  ULONG returned = 0;
  int index = 0;

  while (enumerator->Next(WBEM_INFINITE, 1, &obj, &returned) == S_OK) {
    GPUInfo gpu;
    gpu.adapterIndex = index++;

    VARIANT var;

    if (SUCCEEDED(obj->Get(L"Name", 0, &var, 0, 0))) {
      if (var.vt == VT_BSTR) {
        _bstr_t bstr(var.bstrVal);
        gpu.name = (const char *)bstr;
      }
      VariantClear(&var);
    }

    if (SUCCEEDED(obj->Get(L"AdapterRAM", 0, &var, 0, 0))) {
      // AdapterRAM returns signed 32-bit, might be incorrect for >2GB
      if (var.vt == VT_I4 || var.vt == VT_UI4) {
        gpu.vramBytes = (uint64_t)(unsigned int)var.uintVal;
      }
      VariantClear(&var);
    }

    if (SUCCEEDED(obj->Get(L"DriverVersion", 0, &var, 0, 0))) {
      if (var.vt == VT_BSTR) {
        _bstr_t bstr(var.bstrVal);
        gpu.driverVersion = (const char *)bstr;
      }
      VariantClear(&var);
    }

    // Detect vendor from name
    if (gpu.name.find("NVIDIA") != std::string::npos) {
      gpu.vendor = "NVIDIA";
    } else if (gpu.name.find("AMD") != std::string::npos ||
               gpu.name.find("Radeon") != std::string::npos) {
      gpu.vendor = "AMD";
    } else if (gpu.name.find("Intel") != std::string::npos) {
      gpu.vendor = "Intel";
    }

    // Try to parse VRAM from GPU name (e.g., "RTX 3050 6GB Laptop GPU")
    // This is more reliable than WMI's AdapterRAM for many cards
    size_t gbPos = gpu.name.find("GB");
    if (gbPos != std::string::npos && gbPos > 0) {
      // Look backwards for the number
      size_t numStart = gbPos - 1;
      while (numStart > 0 && (isdigit(gpu.name[numStart - 1]) ||
                              gpu.name[numStart - 1] == ' ')) {
        numStart--;
      }
      std::string numStr;
      for (size_t i = numStart; i < gbPos; i++) {
        if (isdigit(gpu.name[i])) {
          numStr += gpu.name[i];
        }
      }
      if (!numStr.empty()) {
        int vramGB = std::stoi(numStr);
        if (vramGB > 0 && vramGB <= 48) { // Sanity check: 1-48 GB range
          gpu.vramBytes = (uint64_t)vramGB * 1024 * 1024 * 1024;
        }
      }
    }

    gpus.push_back(gpu);
    obj->Release();
  }

  enumerator->Release();

  // Sort GPUs to prioritize discrete over integrated
  // Discrete GPUs (NVIDIA, AMD discrete) should come first
  std::sort(gpus.begin(), gpus.end(), [](const GPUInfo &a, const GPUInfo &b) {
    // NVIDIA discrete is highest priority
    bool aIsNvidia = (a.vendor == "NVIDIA");
    bool bIsNvidia = (b.vendor == "NVIDIA");
    if (aIsNvidia && !bIsNvidia)
      return true;
    if (!aIsNvidia && bIsNvidia)
      return false;

    // Check if AMD is discrete (not integrated Radeon Graphics)
    bool aIsIntegrated = (a.name.find("Graphics") != std::string::npos &&
                          a.name.find("Radeon") != std::string::npos &&
                          a.name.find("RX") == std::string::npos);
    bool bIsIntegrated = (b.name.find("Graphics") != std::string::npos &&
                          b.name.find("Radeon") != std::string::npos &&
                          b.name.find("RX") == std::string::npos);

    if (!aIsIntegrated && bIsIntegrated)
      return true;
    if (aIsIntegrated && !bIsIntegrated)
      return false;

    // Prefer GPU with more VRAM
    return a.vramBytes > b.vramBytes;
  });

  return gpus;
}

HardwareDetector::RAMInfo HardwareDetector::DetectRAM() {
  RAMInfo info;

  // Get total/available from GlobalMemoryStatusEx
  MEMORYSTATUSEX memStatus;
  memStatus.dwLength = sizeof(memStatus);
  if (GlobalMemoryStatusEx(&memStatus)) {
    info.totalBytes = memStatus.ullTotalPhys;
    info.availableBytes = memStatus.ullAvailPhys;
  }

  // Get speed and type from WMI
  if (wmiInitialized) {
    IWbemServices *services = (IWbemServices *)wbemServices;
    IEnumWbemClassObject *enumerator = nullptr;

    HRESULT hr = services->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT Speed, SMBIOSMemoryType FROM Win32_PhysicalMemory"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL,
        &enumerator);

    if (SUCCEEDED(hr)) {
      IWbemClassObject *obj = nullptr;
      ULONG returned = 0;

      while (enumerator->Next(WBEM_INFINITE, 1, &obj, &returned) == S_OK) {
        VARIANT var;
        info.moduleCount++;

        if (info.speedMHz == 0 &&
            SUCCEEDED(obj->Get(L"Speed", 0, &var, 0, 0))) {
          if (var.vt == VT_I4 || var.vt == VT_UI4) {
            info.speedMHz = var.intVal;
          }
          VariantClear(&var);
        }

        if (info.type.empty() &&
            SUCCEEDED(obj->Get(L"SMBIOSMemoryType", 0, &var, 0, 0))) {
          if (var.vt == VT_I4 || var.vt == VT_UI4) {
            int memType = var.intVal;
            // SMBIOS memory types
            switch (memType) {
            case 26:
              info.type = "DDR4";
              break;
            case 34:
              info.type = "DDR5";
              break;
            case 24:
              info.type = "DDR3";
              break;
            default:
              info.type = "DDR";
              break;
            }
          }
          VariantClear(&var);
        }

        obj->Release();
      }
      enumerator->Release();
    }
  }

  return info;
}

std::vector<HardwareDetector::StorageInfo> HardwareDetector::DetectStorage() {
  std::vector<StorageInfo> storage;

  if (!wmiInitialized)
    return storage;

  IWbemServices *services = (IWbemServices *)wbemServices;
  IEnumWbemClassObject *enumerator = nullptr;

  HRESULT hr = services->ExecQuery(
      bstr_t("WQL"),
      bstr_t("SELECT Model, Size, MediaType FROM Win32_DiskDrive"),
      WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &enumerator);

  if (FAILED(hr))
    return storage;

  IWbemClassObject *obj = nullptr;
  ULONG returned = 0;

  while (enumerator->Next(WBEM_INFINITE, 1, &obj, &returned) == S_OK) {
    StorageInfo disk;
    VARIANT var;

    if (SUCCEEDED(obj->Get(L"Model", 0, &var, 0, 0))) {
      if (var.vt == VT_BSTR) {
        _bstr_t bstr(var.bstrVal);
        disk.name = (const char *)bstr;
      }
      VariantClear(&var);
    }

    if (SUCCEEDED(obj->Get(L"Size", 0, &var, 0, 0))) {
      if (var.vt == VT_BSTR) {
        disk.sizeBytes = _wtoi64(var.bstrVal);
      }
      VariantClear(&var);
    }

    // Check if NVMe or SSD
    if (disk.name.find("NVMe") != std::string::npos) {
      disk.type = "SSD";
      disk.interfaceType = "NVMe";
    } else if (disk.name.find("SSD") != std::string::npos) {
      disk.type = "SSD";
      disk.interfaceType = "SATA";
    } else {
      disk.type = "HDD";
      disk.interfaceType = "SATA";
    }

    storage.push_back(disk);
    obj->Release();
  }

  enumerator->Release();
  return storage;
}

std::string HardwareDetector::DetectOSVersion() {
  std::string version;

  if (!wmiInitialized)
    return version;

  IWbemServices *services = (IWbemServices *)wbemServices;
  IEnumWbemClassObject *enumerator = nullptr;

  HRESULT hr = services->ExecQuery(
      bstr_t("WQL"),
      bstr_t("SELECT Caption, Version FROM Win32_OperatingSystem"),
      WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &enumerator);

  if (FAILED(hr))
    return version;

  IWbemClassObject *obj = nullptr;
  ULONG returned = 0;

  if (enumerator->Next(WBEM_INFINITE, 1, &obj, &returned) == S_OK) {
    VARIANT var;

    if (SUCCEEDED(obj->Get(L"Caption", 0, &var, 0, 0))) {
      if (var.vt == VT_BSTR) {
        _bstr_t bstr(var.bstrVal);
        version = (const char *)bstr;
      }
      VariantClear(&var);
    }

    obj->Release();
  }

  enumerator->Release();
  return version;
}

std::string HardwareDetector::QueryWMIString(const wchar_t *wqlQuery,
                                             const wchar_t *property) {
  std::string result;
  if (!wmiInitialized)
    return result;

  IWbemServices *services = (IWbemServices *)wbemServices;
  IEnumWbemClassObject *enumerator = nullptr;

  HRESULT hr = services->ExecQuery(
      bstr_t("WQL"), bstr_t(wqlQuery),
      WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &enumerator);

  if (FAILED(hr))
    return result;

  IWbemClassObject *obj = nullptr;
  ULONG returned = 0;

  if (enumerator->Next(WBEM_INFINITE, 1, &obj, &returned) == S_OK) {
    VARIANT var;
    if (SUCCEEDED(obj->Get(property, 0, &var, 0, 0))) {
      if (var.vt == VT_BSTR) {
        _bstr_t bstr(var.bstrVal);
        result = (const char *)bstr;
      }
      VariantClear(&var);
    }
    obj->Release();
  }

  enumerator->Release();
  return result;
}

uint64_t HardwareDetector::QueryWMINumber(const wchar_t *wqlQuery,
                                          const wchar_t *property) {
  uint64_t result = 0;
  if (!wmiInitialized)
    return result;

  IWbemServices *services = (IWbemServices *)wbemServices;
  IEnumWbemClassObject *enumerator = nullptr;

  HRESULT hr = services->ExecQuery(
      bstr_t("WQL"), bstr_t(wqlQuery),
      WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &enumerator);

  if (FAILED(hr))
    return result;

  IWbemClassObject *obj = nullptr;
  ULONG returned = 0;

  if (enumerator->Next(WBEM_INFINITE, 1, &obj, &returned) == S_OK) {
    VARIANT var;
    if (SUCCEEDED(obj->Get(property, 0, &var, 0, 0))) {
      switch (var.vt) {
      case VT_I4:
        result = var.lVal;
        break;
      case VT_UI4:
        result = var.ulVal;
        break;
      case VT_I8:
        result = var.llVal;
        break;
      case VT_UI8:
        result = var.ullVal;
        break;
      case VT_BSTR:
        result = _wtoi64(var.bstrVal);
        break;
      }
      VariantClear(&var);
    }
    obj->Release();
  }

  enumerator->Release();
  return result;
}

std::vector<std::pair<std::string, uint64_t>>
HardwareDetector::QueryWMIMultiple(const wchar_t *wqlQuery,
                                   const wchar_t *stringProp,
                                   const wchar_t *numProp) {
  std::vector<std::pair<std::string, uint64_t>> results;
  if (!wmiInitialized)
    return results;

  IWbemServices *services = (IWbemServices *)wbemServices;
  IEnumWbemClassObject *enumerator = nullptr;

  HRESULT hr = services->ExecQuery(
      bstr_t("WQL"), bstr_t(wqlQuery),
      WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &enumerator);

  if (FAILED(hr))
    return results;

  IWbemClassObject *obj = nullptr;
  ULONG returned = 0;

  while (enumerator->Next(WBEM_INFINITE, 1, &obj, &returned) == S_OK) {
    std::string str;
    uint64_t num = 0;
    VARIANT var;

    if (SUCCEEDED(obj->Get(stringProp, 0, &var, 0, 0))) {
      if (var.vt == VT_BSTR) {
        _bstr_t bstr(var.bstrVal);
        str = (const char *)bstr;
      }
      VariantClear(&var);
    }

    if (SUCCEEDED(obj->Get(numProp, 0, &var, 0, 0))) {
      switch (var.vt) {
      case VT_I4:
        num = var.lVal;
        break;
      case VT_UI4:
        num = var.ulVal;
        break;
      case VT_I8:
        num = var.llVal;
        break;
      case VT_UI8:
        num = var.ullVal;
        break;
      }
      VariantClear(&var);
    }

    results.push_back({str, num});
    obj->Release();
  }

  enumerator->Release();
  return results;
}
