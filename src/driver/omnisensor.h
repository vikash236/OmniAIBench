/*
 * OmniSensor Shared Header
 * Defines IOCTL codes and structures for driver-usermode communication
 * License: MIT
 */

#pragma once

#ifdef _KERNEL_MODE
#include <ntddk.h>
#else
#include <windows.h>
#include <winioctl.h>
#endif

// Device and symbolic link names
#define OMNISENSOR_DEVICE_NAME L"\\Device\\OmniSensor"
#define OMNISENSOR_SYMBOLIC_LINK L"\\DosDevices\\OmniSensor"
#define OMNISENSOR_USER_DEVICE L"\\\\.\\OmniSensor"

// IOCTL definitions
#define FILE_DEVICE_OMNISENSOR 0x8000

#define IOCTL_READ_MSR                                                         \
  CTL_CODE(FILE_DEVICE_OMNISENSOR, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WRITE_MSR                                                        \
  CTL_CODE(FILE_DEVICE_OMNISENSOR, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_READ_PCI                                                         \
  CTL_CODE(FILE_DEVICE_OMNISENSOR, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_READ_IO_PORT                                                     \
  CTL_CODE(FILE_DEVICE_OMNISENSOR, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_CPU_TEMP                                                     \
  CTL_CODE(FILE_DEVICE_OMNISENSOR, 0x810, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_CPU_POWER                                                    \
  CTL_CODE(FILE_DEVICE_OMNISENSOR, 0x811, METHOD_BUFFERED, FILE_ANY_ACCESS)

// MSR addresses for various CPU sensors
#define MSR_RAPL_POWER_UNIT 0x606      // Intel power units
#define MSR_PKG_ENERGY_STATUS 0x611    // Intel package energy
#define MSR_DRAM_ENERGY_STATUS 0x619   // Intel DRAM energy
#define MSR_PP0_ENERGY_STATUS 0x639    // Intel core energy
#define MSR_PP1_ENERGY_STATUS 0x641    // Intel GPU/uncore energy
#define MSR_PACKAGE_THERM_STATUS 0x1B1 // Intel thermal status
#define MSR_TEMPERATURE_TARGET 0x1A2   // Intel TjMax
#define MSR_IA32_THERM_STATUS 0x19C    // Intel core thermal

// AMD-specific MSRs
#define AMD_MSR_PSTATE_0 0xC0010064    // P-state 0 (max freq)
#define AMD_MSR_CORE_ENERGY 0xC001029A // Core energy
#define AMD_MSR_PKG_ENERGY 0xC001029B  // Package energy
#define AMD_MSR_HWCR 0xC0010015        // Hardware config

// SMN (System Management Network) for AMD Ryzen temperature
// Accessed via PCI config space 0x60/0x64
#define AMD_SMN_THM_TCON_CUR_TMP 0x00059800 // Current temperature
#define AMD_SMN_THM_CTF_LIMIT 0x00059804    // CTF limit

// Request structures
#pragma pack(push, 1)

typedef struct _MSR_REQUEST {
  ULONG32 Register;  // MSR register address
  ULONG32 CoreIndex; // CPU core to read from (0-based)
  ULONG64 Value;     // Value read or to write
  LONG Status;       // NTSTATUS result
} MSR_REQUEST, *PMSR_REQUEST;

typedef struct _PCI_REQUEST {
  ULONG32 Bus;
  ULONG32 Device;
  ULONG32 Function;
  ULONG32 Offset;
  ULONG32 Size; // 1, 2, or 4 bytes
  ULONG32 Value;
  LONG Status;
} PCI_REQUEST, *PPCI_REQUEST;

typedef struct _IO_PORT_REQUEST {
  USHORT Port;
  UCHAR Size; // 1, 2, or 4 bytes
  ULONG32 Value;
  LONG Status;
} IO_PORT_REQUEST, *PIO_PORT_REQUEST;

typedef struct _CPU_TEMP_RESULT {
  ULONG32 CoreCount;
  FLOAT CoreTemps[64]; // Temperature per core in Celsius
  FLOAT PackageTemp;   // Package/Tdie temperature
  FLOAT TjMax;         // Max junction temperature
  LONG Status;
} CPU_TEMP_RESULT, *PCPU_TEMP_RESULT;

typedef struct _CPU_POWER_RESULT {
  FLOAT PackagePowerW; // Package power in Watts
  FLOAT CorePowerW;    // Core power in Watts
  FLOAT DramPowerW;    // DRAM power in Watts (Intel only)
  FLOAT GpuPowerW;     // Integrated GPU power (Intel only)
  FLOAT EnergyUnits;   // Energy units scale factor
  LONG Status;
} CPU_POWER_RESULT, *PCPU_POWER_RESULT;

#pragma pack(pop)

// User-mode helper macros
#ifndef _KERNEL_MODE

inline HANDLE OpenOmniSensor() {
  return CreateFileW(OMNISENSOR_USER_DEVICE, GENERIC_READ | GENERIC_WRITE, 0,
                     NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

inline BOOL ReadMSR(HANDLE hDevice, ULONG32 msrAddress, ULONG32 coreIndex,
                    PULONG64 value) {
  MSR_REQUEST request = {0};
  request.Register = msrAddress;
  request.CoreIndex = coreIndex;

  DWORD bytesReturned;
  BOOL result =
      DeviceIoControl(hDevice, IOCTL_READ_MSR, &request, sizeof(request),
                      &request, sizeof(request), &bytesReturned, NULL);

  if (result && request.Status == 0) {
    *value = request.Value;
    return TRUE;
  }
  return FALSE;
}

#endif // !_KERNEL_MODE
