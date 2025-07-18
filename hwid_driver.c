#include <ntifs.h>
#include <ntddk.h>
#include <ntddstor.h>
#include <ndis.h>

#define DEVICE_NAME L"\\Device\\HWIDSpoofer"
#define SYMBOLIC_NAME L"\\DosDevices\\HWIDSpoofer"
#define MAX_SERIAL_LENGTH 128
#define MAX_MAC_LENGTH 32

// Global variables
PDRIVER_DISPATCH OriginalDiskDispatch = NULL;
PVOID OriginalNdisQuery = NULL;
CHAR SpoofedDiskSerial[MAX_SERIAL_LENGTH] = { 0 };
CHAR SpoofedMacAddress[MAX_MAC_LENGTH] = { 0 };
BOOLEAN IsSpoofingDisk = FALSE;
BOOLEAN IsSpoofingMac = FALSE;

// Function to create device object
NTSTATUS CreateDevice(PDRIVER_OBJECT DriverObject) {
    UNICODE_STRING deviceName, symbolicName;
    PDEVICE_OBJECT deviceObject = NULL;
    NTSTATUS status;

    RtlInitUnicodeString(&deviceName, DEVICE_NAME);
    RtlInitUnicodeString(&symbolicName, SYMBOLIC_NAME);

    status = IoCreateDevice(DriverObject, 0, &deviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &deviceObject);
    if (!NT_SUCCESS(status)) {
        DbgPrint("HWIDSpoofer: Failed to create device (0x%08X)\n", status);
        return status;
    }

    status = IoCreateSymbolicLink(&symbolicName, &deviceName);
    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(deviceObject);
        DbgPrint("HWIDSpoofer: Failed to create symbolic link (0x%08X)\n", status);
        return status;
    }

    deviceObject->Flags |= DO_BUFFERED_IO;
    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    return STATUS_SUCCESS;
}

// Function to handle IOCTLs from user-mode
NTSTATUS DeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status = STATUS_SUCCESS;
    ULONG bytesReturned = 0;

    switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_SET_SPOOFED_DISK_SERIAL:
        if (irpStack->Parameters.DeviceIoControl.InputBufferLength < MAX_SERIAL_LENGTH) {
            RtlCopyMemory(SpoofedDiskSerial, Irp->AssociatedIrp.SystemBuffer, irpStack->Parameters.DeviceIoControl.InputBufferLength);
            IsSpoofingDisk = TRUE;
            DbgPrint("HWIDSpoofer: Set spoofed disk serial: %s\n", SpoofedDiskSerial);
        }
        else {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        bytesReturned = 0;
        break;

    case IOCTL_CLEAR_SPOOFED_DISK_SERIAL:
        RtlZeroMemory(SpoofedDiskSerial, MAX_SERIAL_LENGTH);
        IsSpoofingDisk = FALSE;
        DbgPrint("HWIDSpoofer: Cleared spoofed disk serial\n");
        bytesReturned = 0;
        break;

    case IOCTL_SET_SPOOFED_MAC_ADDRESS:
        if (irpStack->Parameters.DeviceIoControl.InputBufferLength < MAX_MAC_LENGTH) {
            RtlCopyMemory(SpoofedMacAddress, Irp->AssociatedIrp.SystemBuffer, irpStack->Parameters.DeviceIoControl.InputBufferLength);
            IsSpoofingMac = TRUE;
            DbgPrint("HWIDSpoofer: Set spoofed MAC address: %s\n", SpoofedMacAddress);
        }
        else {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        bytesReturned = 0;
        break;

    case IOCTL_CLEAR_SPOOFED_MAC_ADDRESS:
        RtlZeroMemory(SpoofedMacAddress, MAX_MAC_LENGTH);
        IsSpoofingMac = FALSE;
        DbgPrint("HWIDSpoofer: Cleared spoofed MAC address\n");
        bytesReturned = 0;
        break;

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = bytesReturned;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

// Hooked disk.sys dispatch for IOCTL_STORAGE_QUERY_PROPERTY
NTSTATUS HookedDiskDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

    if (irpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_STORAGE_QUERY_PROPERTY) {
        PSTORAGE_PROPERTY_QUERY query = (PSTORAGE_PROPERTY_QUERY)Irp->AssociatedIrp.SystemBuffer;
        if (query->PropertyId == StorageDeviceProperty && IsSpoofingDisk) {
            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(STORAGE_DEVICE_DESCRIPTOR) + MAX_SERIAL_LENGTH) {
                PSTORAGE_DEVICE_DESCRIPTOR descriptor = (PSTORAGE_DEVICE_DESCRIPTOR)Irp->AssociatedIrp.SystemBuffer;
                descriptor->Version = sizeof(STORAGE_DEVICE_DESCRIPTOR);
                descriptor->Size = sizeof(STORAGE_DEVICE_DESCRIPTOR) + MAX_SERIAL_LENGTH;
                descriptor->SerialNumberOffset = sizeof(STORAGE_DEVICE_DESCRIPTOR);
                RtlCopyMemory((PUCHAR)descriptor + descriptor->SerialNumberOffset, SpoofedDiskSerial, strlen(SpoofedDiskSerial) + 1);

                Irp->IoStatus.Status = STATUS_SUCCESS;
                Irp->IoStatus.Information = sizeof(STORAGE_DEVICE_DESCRIPTOR) + strlen(SpoofedDiskSerial) + 1;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                DbgPrint("HWIDSpoofer: Returned spoofed disk serial: %s\n", SpoofedDiskSerial);
                return STATUS_SUCCESS;
            }
        }
    }

    return OriginalDiskDispatch(DeviceObject, Irp);
}

// Hooked NDIS query for MAC address
NDIS_STATUS HookedNdisQuery(PVOID MiniportAdapterContext, NDIS_OID Oid, PVOID InformationBuffer, ULONG InformationBufferLength, PULONG BytesWritten, PULONG BytesNeeded) {
    if (Oid == OID_802_3_PERMANENT_ADDRESS && IsSpoofingMac) {
        if (InformationBufferLength >= 6) {
            UCHAR mac[6];
            char* token = strtok(SpoofedMacAddress, ":");
            for (int i = 0; i < 6 && token; ++i) {
                mac[i] = (UCHAR)strtol(token, NULL, 16);
                token = strtok(NULL, ":");
            }
            RtlCopyMemory(InformationBuffer, mac, 6);
            *BytesWritten = 6;
            DbgPrint("HWIDSpoofer: Returned spoofed MAC address: %s\n", SpoofedMacAddress);
            return NDIS_STATUS_SUCCESS;
        }
    }

    // Call original NDIS handler (simplified, requires actual function pointer)
    return NDIS_STATUS_SUCCESS;
}

// Function to hook disk.sys dispatch
NTSTATUS HookDiskDispatch() {
    UNICODE_STRING driverName;
    PDRIVER_OBJECT diskDriver;

    RtlInitUnicodeString(&driverName, L"\\Driver\\Disk");
    NTSTATUS status = ObReferenceObjectByName(&driverName, OBJ_CASE_INSENSITIVE, NULL, 0,
        *IoDriverObjectType, KernelMode, NULL, (PVOID*)&diskDriver);

    if (!NT_SUCCESS(status)) {
        DbgPrint("HWIDSpoofer: Failed to get disk driver (0x%08X)\n", status);
        return status;
    }

    OriginalDiskDispatch = diskDriver->MajorFunction[IRP_MJ_DEVICE_CONTROL];
    diskDriver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = HookedDiskDispatch;

    ObDereferenceObject(diskDriver);
    return STATUS_SUCCESS;
}

// Function to hook NDIS (simplified)
NTSTATUS HookNdisQuery() {
    DbgPrint("HWIDSpoofer: NDIS hooking not fully implemented in this example\n");
    return STATUS_SUCCESS;
}

// Driver unload routine
VOID DriverUnload(PDRIVER_OBJECT DriverObject) {
    UNICODE_STRING symbolicName;
    RtlInitUnicodeString(&symbolicName, SYMBOLIC_NAME);
    IoDeleteSymbolicLink(&symbolicName);
    IoDeleteDevice(DriverObject->DeviceObject);

    if (OriginalDiskDispatch) {
        UNICODE_STRING driverName;
        PDRIVER_OBJECT diskDriver;
        RtlInitUnicodeString(&driverName, L"\\Driver\\Disk");
        if (NT_SUCCESS(ObReferenceObjectByName(&driverName, OBJ_CASE_INSENSITIVE, NULL, 0,
            *IoDriverObjectType, KernelMode, NULL, (PVOID*)&diskDriver))) {
            diskDriver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = OriginalDiskDispatch;
            ObDereferenceObject(diskDriver);
        }
    }

    if (OriginalNdisQuery) {
        DbgPrint("HWIDSpoofer: Restored NDIS handler (placeholder)\n");
    }

    DbgPrint("HWIDSpoofer: Driver unloaded\n");
}

// Driver entry point
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
    UNREFERENCED_PARAMETER(RegistryPath);
    DbgPrint("HWIDSpoofer: Driver loaded\n");

    NTSTATUS status = CreateDevice(DriverObject);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = HookDiskDispatch();
    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(DriverObject->DeviceObject);
        return status;
    }

    status = HookNdisQuery();
    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(DriverObject->DeviceObject);
        return status;
    }

    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceControl;
    DriverObject->DriverUnload = DriverUnload;

    return STATUS_SUCCESS;
}