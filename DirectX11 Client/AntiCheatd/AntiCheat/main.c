/**
 * @file FlectDriver.c
 * @brief Kernel-mode driver entry point and IOCTL dispatching.
 * @details Initializes the device object, symbolic links, and routes user-mode 
 * requests (HWID, Security) to appropriate kernel routines.
 */

#include "GetHardware/GetHardware.h"
#include "ProcessGuard/ProcessGuard.h"
#include "Context/HwSecurityProtocol/HwSecurityProtocol.h"
#include "Context/HardWareContext/HardWareContext.h"
#include "crc/crc.h"

// --- Global Driver State ---
BOOLEAN g_DriverActive;            /**< Driver operational state. */
UNICODE_STRING g_DeviceName;       /**< Kernel-mode device object name. */
UNICODE_STRING g_SymbolicLinkName; /**< User-mode visible symbolic link name. */
PDEVICE_OBJECT g_DeviceObject;     /**< Pointer to the created device object. */

// --- Function Declarations ---
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
VOID DriverUnload(PDRIVER_OBJECT DriverObject);
NTSTATUS DeviceCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS DeviceClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS DeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

/**
 * @brief Driver entry point: Initializes device objects and communication channels.
 */
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    NTSTATUS status;
    UNREFERENCED_PARAMETER(RegistryPath);

    // Initialize device and symbolic link names
    RtlInitUnicodeString(&g_DeviceName, L"\\Device\\Flect");
    RtlInitUnicodeString(&g_SymbolicLinkName, L"\\DosDevices\\Flect");

    // Create the device object accessible by the system
    status = IoCreateDevice(
        DriverObject,
        0,
        &g_DeviceName,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &g_DeviceObject
    );

    if (!NT_SUCCESS(status)) return status;

    // Create symbolic link (allows user-mode to access via \\.\Flect)
    status = IoCreateSymbolicLink(&g_SymbolicLinkName, &g_DeviceName);
    if (!NT_SUCCESS(status)) {
        DbgPrint("[DriverEntry] Failed to create symbolic link: 0x%08X\n", status);
        IoDeleteDevice(g_DeviceObject);
        return status;
    }

    // Register IRP major function handlers
    DriverObject->MajorFunction[IRP_MJ_CREATE] = DeviceCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DeviceClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceControl;
    DriverObject->DriverUnload = DriverUnload;

    g_DriverActive = TRUE;
    DbgPrint("Flect Driver was successfully loaded to check the program\n");

    return STATUS_SUCCESS;
}

/**
 * @brief Handles driver unloading: Clean up callbacks and devices.
 */
VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);

    // Disable process protection callbacks
    if (g_ProtectionEnabled) {
        DisableProcessProtection();
    }

    g_DriverActive = FALSE;

    // Cleanup device links and objects
    IoDeleteSymbolicLink(&g_SymbolicLinkName);
    if (g_DeviceObject) {
        IoDeleteDevice(g_DeviceObject);
    }

    DbgPrint("HardwareInfo Driver unloaded\n");
}

/**
 * @brief Device create/close stubs (Handle open/close management).
 */
NTSTATUS DeviceCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    DbgPrint("HardwareInfo Device opened\n");
    return STATUS_SUCCESS;
}

NTSTATUS DeviceClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    DbgPrint("HardwareInfo Device closed\n");
    return STATUS_SUCCESS;
}

/**
 * @brief Central dispatch routine for all IOCTL communication.
 * @details Routes user-mode DeviceIoControl requests to kernel functions.
 */
NTSTATUS DeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpStack;
    ULONG ioControlCode;
    PVOID inputBuffer, outputBuffer;
    ULONG inputLength, outputLength;
    ULONG bytesTransferred = 0;

    UNREFERENCED_PARAMETER(DeviceObject);

    if (!g_DriverActive) {
        Irp->IoStatus.Status = STATUS_DEVICE_NOT_READY;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_DEVICE_NOT_READY;
    }

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;
    inputBuffer = Irp->AssociatedIrp.SystemBuffer;
    outputBuffer = Irp->AssociatedIrp.SystemBuffer;
    inputLength = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outputLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    switch (ioControlCode)
    {
    case IOCTL_HARDWARE_GET_INFO:
    {
        if (inputLength < sizeof(HARDWARE_REQUEST)) {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        PHARDWARE_REQUEST request = (PHARDWARE_REQUEST)inputBuffer;
        PUCHAR responseMessage = NULL;
        ULONG messageSize = 0;

        // Perform hardware fingerprinting
        status = BuildHardwareResponseMessage(request->RequestType, &responseMessage, &messageSize);

        if (NT_SUCCESS(status)) {
            if (outputLength >= messageSize) {
                RtlCopyMemory(outputBuffer, responseMessage, messageSize);
                bytesTransferred = messageSize;
            } else {
                status = STATUS_BUFFER_TOO_SMALL;
            }
            if (responseMessage) ExFreePool(responseMessage);
        }
    }
    break;

    case IOCTL_HARDWARE_HEARTBEAT:
    {
        ULONG heartbeatResponse = 0x12345678;
        if (outputLength >= sizeof(ULONG)) {
            *(PULONG)outputBuffer = heartbeatResponse;
            bytesTransferred = sizeof(ULONG);
        } else {
            status = STATUS_BUFFER_TOO_SMALL;
        }
    }
    break;

    case IOCTL_SECURITY_CONTROL:
    {
        if (inputLength < sizeof(PROTECTION_REQUEST)) {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        PPROTECTION_REQUEST protRequest = (PPROTECTION_REQUEST)inputBuffer;
        switch (protRequest->RequestType) {
        case SECURITY_REQUEST_OB_REGISTER:
            status = EnableProcessProtection();
            break;
        case SECURITY_REQUEST_ADD_PID:
            status = AddProtectedProcess(protRequest->ProcessId);
            break;
        case SECURITY_REQUEST_GET_ALERTS:
            status = GetAlert_Queue(outputBuffer, outputLength, &bytesTransferred);
            break;
        default:
            status = STATUS_INVALID_PARAMETER;
            break;
        }
    }
    break;

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = bytesTransferred;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}
