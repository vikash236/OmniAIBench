/*
 * OmniSensor Kernel Driver
 * Direct hardware access for accurate sensor readings (like HWiNFO64)
 * License: MIT
 *
 * IMPORTANT: Requires Windows Driver Kit (WDK) to compile
 * Install WDK:
 * https://docs.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk
 */

#include <intrin.h>
#include <ntddk.h>


// Device name and symbolic link
#define DEVICE_NAME L"\\Device\\OmniSensor"
#define SYMBOLIC_LINK L"\\DosDevices\\OmniSensor"

// IOCTL codes for user-mode communication
#define FILE_DEVICE_OMNISENSOR 0x8000
#define IOCTL_READ_MSR                                                         \
  CTL_CODE(FILE_DEVICE_OMNISENSOR, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WRITE_MSR                                                        \
  CTL_CODE(FILE_DEVICE_OMNISENSOR, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_READ_PCI                                                         \
  CTL_CODE(FILE_DEVICE_OMNISENSOR, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_READ_IO_PORT                                                     \
  CTL_CODE(FILE_DEVICE_OMNISENSOR, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)

// MSR addresses for CPU temperature/power (AMD Ryzen / Intel Core)
#define MSR_RAPL_POWER_UNIT 0x606      // Power units
#define MSR_PKG_ENERGY_STATUS 0x611    // Package energy
#define MSR_PACKAGE_THERM_STATUS 0x1B1 // Intel thermal status
#define MSR_TEMPERATURE_TARGET 0x1A2   // Intel temp target
#define AMD_MSR_TEMPERATURE 0xC0010063 // AMD CPU temp

// Structures for IOCTL communication
typedef struct _MSR_REQUEST {
  ULONG32 Register;  // MSR register address
  ULONG32 CoreIndex; // CPU core to read from
  ULONG64 Value;     // Value read/written
  NTSTATUS Status;   // Operation status
} MSR_REQUEST, *PMSR_REQUEST;

typedef struct _PCI_REQUEST {
  ULONG32 Bus;
  ULONG32 Device;
  ULONG32 Function;
  ULONG32 Offset;
  ULONG32 Size; // 1, 2, or 4 bytes
  ULONG32 Value;
  NTSTATUS Status;
} PCI_REQUEST, *PPCI_REQUEST;

// Global device object
PDEVICE_OBJECT g_DeviceObject = NULL;

// Forward declarations
DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;
NTSTATUS CreateDevice(PDRIVER_OBJECT DriverObject);
NTSTATUS DeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS DeviceCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS DeviceClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);

// Read MSR on specific CPU core
NTSTATUS ReadMsrOnCore(ULONG32 MsrAddress, ULONG32 CoreIndex, PULONG64 Value) {
  KAFFINITY oldAffinity;
  KAFFINITY newAffinity;

  // Set CPU affinity to target core
  newAffinity = (KAFFINITY)(1ULL << CoreIndex);
  oldAffinity = KeSetSystemAffinityThreadEx(newAffinity);

  __try {
    *Value = __readmsr(MsrAddress);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    KeRevertToUserAffinityThreadEx(oldAffinity);
    return STATUS_ACCESS_VIOLATION;
  }

  KeRevertToUserAffinityThreadEx(oldAffinity);
  return STATUS_SUCCESS;
}

// Write MSR on specific CPU core
NTSTATUS WriteMsrOnCore(ULONG32 MsrAddress, ULONG32 CoreIndex, ULONG64 Value) {
  KAFFINITY oldAffinity;
  KAFFINITY newAffinity;

  newAffinity = (KAFFINITY)(1ULL << CoreIndex);
  oldAffinity = KeSetSystemAffinityThreadEx(newAffinity);

  __try {
    __writemsr(MsrAddress, Value);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    KeRevertToUserAffinityThreadEx(oldAffinity);
    return STATUS_ACCESS_VIOLATION;
  }

  KeRevertToUserAffinityThreadEx(oldAffinity);
  return STATUS_SUCCESS;
}

// Device control handler (IOCTL processing)
NTSTATUS DeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
  UNREFERENCED_PARAMETER(DeviceObject);

  PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(Irp);
  ULONG ioControlCode = ioStack->Parameters.DeviceIoControl.IoControlCode;
  PVOID buffer = Irp->AssociatedIrp.SystemBuffer;
  ULONG inputLength = ioStack->Parameters.DeviceIoControl.InputBufferLength;
  ULONG outputLength = ioStack->Parameters.DeviceIoControl.OutputBufferLength;

  NTSTATUS status = STATUS_SUCCESS;
  ULONG bytesReturned = 0;

  switch (ioControlCode) {
  case IOCTL_READ_MSR: {
    if (inputLength < sizeof(MSR_REQUEST) ||
        outputLength < sizeof(MSR_REQUEST)) {
      status = STATUS_BUFFER_TOO_SMALL;
      break;
    }

    PMSR_REQUEST request = (PMSR_REQUEST)buffer;
    request->Status =
        ReadMsrOnCore(request->Register, request->CoreIndex, &request->Value);
    bytesReturned = sizeof(MSR_REQUEST);
    break;
  }

  case IOCTL_WRITE_MSR: {
    if (inputLength < sizeof(MSR_REQUEST)) {
      status = STATUS_BUFFER_TOO_SMALL;
      break;
    }

    PMSR_REQUEST request = (PMSR_REQUEST)buffer;
    request->Status =
        WriteMsrOnCore(request->Register, request->CoreIndex, request->Value);
    bytesReturned = sizeof(MSR_REQUEST);
    break;
  }

  default:
    status = STATUS_INVALID_DEVICE_REQUEST;
    break;
  }

  Irp->IoStatus.Status = status;
  Irp->IoStatus.Information = bytesReturned;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return status;
}

// Device create handler
NTSTATUS DeviceCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
  UNREFERENCED_PARAMETER(DeviceObject);

  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return STATUS_SUCCESS;
}

// Device close handler
NTSTATUS DeviceClose(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
  UNREFERENCED_PARAMETER(DeviceObject);

  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return STATUS_SUCCESS;
}

// Create device and symbolic link
NTSTATUS CreateDevice(PDRIVER_OBJECT DriverObject) {
  NTSTATUS status;
  UNICODE_STRING deviceName;
  UNICODE_STRING symbolicLink;

  RtlInitUnicodeString(&deviceName, DEVICE_NAME);
  RtlInitUnicodeString(&symbolicLink, SYMBOLIC_LINK);

  // Create device object
  status = IoCreateDevice(DriverObject, 0, &deviceName, FILE_DEVICE_UNKNOWN,
                          FILE_DEVICE_SECURE_OPEN, FALSE, &g_DeviceObject);

  if (!NT_SUCCESS(status)) {
    KdPrint(("OmniSensor: Failed to create device object: 0x%X\n", status));
    return status;
  }

  // Create symbolic link for user-mode access
  status = IoCreateSymbolicLink(&symbolicLink, &deviceName);
  if (!NT_SUCCESS(status)) {
    KdPrint(("OmniSensor: Failed to create symbolic link: 0x%X\n", status));
    IoDeleteDevice(g_DeviceObject);
    return status;
  }

  KdPrint(("OmniSensor: Device created successfully\n"));
  return STATUS_SUCCESS;
}

// Driver unload routine
VOID DriverUnload(PDRIVER_OBJECT DriverObject) {
  UNREFERENCED_PARAMETER(DriverObject);

  UNICODE_STRING symbolicLink;
  RtlInitUnicodeString(&symbolicLink, SYMBOLIC_LINK);

  IoDeleteSymbolicLink(&symbolicLink);

  if (g_DeviceObject) {
    IoDeleteDevice(g_DeviceObject);
  }

  KdPrint(("OmniSensor: Driver unloaded\n"));
}

// Driver entry point
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject,
                     PUNICODE_STRING RegistryPath) {
  UNREFERENCED_PARAMETER(RegistryPath);

  KdPrint(("OmniSensor: Driver loading...\n"));

  // Set up dispatch routines
  DriverObject->DriverUnload = DriverUnload;
  DriverObject->MajorFunction[IRP_MJ_CREATE] = DeviceCreate;
  DriverObject->MajorFunction[IRP_MJ_CLOSE] = DeviceClose;
  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceControl;

  // Create device
  NTSTATUS status = CreateDevice(DriverObject);
  if (!NT_SUCCESS(status)) {
    return status;
  }

  KdPrint(("OmniSensor: Driver loaded successfully\n"));
  return STATUS_SUCCESS;
}
