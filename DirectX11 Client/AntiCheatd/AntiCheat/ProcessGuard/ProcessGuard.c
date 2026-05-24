/**
 * @file ProcessGuard.c
 * @brief Kernel-mode process/thread protection engine.
 * @details Utilizes ObRegisterCallbacks to strip handle access rights and 
 * monitor unauthorized process interactions at the kernel level.
 */

#include "ProcessGuard.h"
#include "../Context/SecurityAccessFlags/SecurityAccessFlags.h"
#include <Ntstrsafe.h>
#include "../crc/crc.h"

// --- Global Security States ---
PVOID g_ObCallbackHandle = NULL;
BOOLEAN g_ProtectionEnabled = FALSE;
PROCESS_PROTECTED g_Process_Protected[10];
ULONG g_ProtectedCount = 0;

// Synchronization primitives for multi-processor safety
FAST_MUTEX g_ProtectionMutex;
KSPIN_LOCK g_AlertLock;

SECURITY_ALERT g_AlertQueue[MAX_ALERTS];
ULONG g_AlertIndex = 0;

/**
 * @brief Registers callback routines to shield processes.
 * @details Configures ObRegisterCallbacks to intercept Handle Creation and Duplication requests.
 */
NTSTATUS EnableProcessProtection(VOID)
{
    NTSTATUS status;
    OB_CALLBACK_REGISTRATION callbackReg;
    OB_OPERATION_REGISTRATION operationReg[2];

    if (g_ProtectionEnabled) return STATUS_SUCCESS;

    ExInitializeFastMutex(&g_ProtectionMutex);
    KeInitializeSpinLock(&g_AlertLock);

    RtlZeroMemory(&callbackReg, sizeof(OB_CALLBACK_REGISTRATION));
    RtlZeroMemory(&operationReg, sizeof(OB_OPERATION_REGISTRATION) * 2);

    callbackReg.Version = ObGetFilterVersion();
    callbackReg.OperationRegistrationCount = 2; 
    RtlInitUnicodeString(&callbackReg.Altitude, L"300000");

    // 1. Process Handle Callbacks
    operationReg[0].ObjectType = PsProcessType;
    operationReg[0].Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
    operationReg[0].PreOperation = PreOperationCallback;
    operationReg[0].PostOperation = PostOperationCallback;

    // 2. Thread Handle Callbacks
    operationReg[1].ObjectType = PsThreadType;
    operationReg[1].Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
    operationReg[1].PreOperation = ThreadPreOperationCallback;
    operationReg[1].PostOperation = NULL;

    callbackReg.OperationRegistration = operationReg;

    status = ObRegisterCallbacks(&callbackReg, &g_ObCallbackHandle);

    if (NT_SUCCESS(status)) {
        g_ProtectionEnabled = TRUE;
        if (NT_SUCCESS(InitializeIntegrityCheck())) {
            DbgPrint("CRC32 Integrity Check Initialized.\n");
        }
    }
    return status;
}

/**
 * @brief Pre-operation callback: Strips malicious access rights.
 * @details If a process attempts to open a handle with restricted flags, access is stripped to 0.
 */
OB_PREOP_CALLBACK_STATUS PreOperationCallback(
    _In_ PVOID RegistrationContext,
    _In_ POB_PRE_OPERATION_INFORMATION OperationInformation
)
{
    UNREFERENCED_PARAMETER(RegistrationContext);
    PEPROCESS TargetProcess = (PEPROCESS)OperationInformation->Object;
    HANDLE TargetPID = PsGetProcessId(TargetProcess);
    HANDLE CurrentPID = PsGetCurrentProcessId();

    if (TargetPID == CurrentPID) return OB_PREOP_SUCCESS;

    if (IsProcessProtected(TargetPID)) {
        if (PsGetProcessId(PsGetCurrentProcess()) == (HANDLE)4) return OB_PREOP_SUCCESS;

        // Handle Creation Check
        if (OperationInformation->Operation == OB_OPERATION_HANDLE_CREATE) {
            if (OperationInformation->Parameters->CreateHandleInformation.OriginalDesiredAccess & PROTECT_FULL_ACCESS) {
                OperationInformation->Parameters->CreateHandleInformation.DesiredAccess = 0;
            }
        }
        // Handle Duplication Check
        else if (OperationInformation->Operation == OB_OPERATION_HANDLE_DUPLICATE) {
            if (OperationInformation->Parameters->DuplicateHandleInformation.OriginalDesiredAccess & PROTECT_FULL_ACCESS) {
                OperationInformation->Parameters->DuplicateHandleInformation.DesiredAccess = 0;
            }
        }
    }
    return OB_PREOP_SUCCESS;
}

/**
 * @brief Post-operation callback: Logs telemetry for security analysis.
 */
VOID PostOperationCallback(
    _In_ PVOID RegistrationContext,
    _In_ POB_POST_OPERATION_INFORMATION OperationInformation
)
{
    UNREFERENCED_PARAMETER(RegistrationContext);
    PEPROCESS TargetProcess = (PEPROCESS)OperationInformation->Object;
    HANDLE TargetPID = PsGetProcessId(TargetProcess);

    if (IsProcessProtected(TargetPID)) {
        PEPROCESS CurrentProcess = PsGetCurrentProcess();
        HANDLE CurrentPID = PsGetCurrentProcessId();

        if (CurrentPID == TargetPID || CurrentPID == (HANDLE)4) return;

        PCHAR processName = (PCHAR)PsGetProcessImageFileName(CurrentProcess);
        ACCESS_MASK grantedAccess = (OperationInformation->Operation == OB_OPERATION_HANDLE_CREATE) ? 
            OperationInformation->Parameters->CreateHandleInformation.GrantedAccess : 
            OperationInformation->Parameters->DuplicateHandleInformation.GrantedAccess;

        AddSecurityAlert(CurrentPID, TargetPID, processName, grantedAccess);
    }
}

/**
 * @brief Thread-level access control.
 */
OB_PREOP_CALLBACK_STATUS ThreadPreOperationCallback(
    _In_ PVOID RegistrationContext,
    _In_ POB_PRE_OPERATION_INFORMATION OperationInformation
)
{
    UNREFERENCED_PARAMETER(RegistrationContext);
    PETHREAD TargetThread = (PETHREAD)OperationInformation->Object;
    PEPROCESS TargetProcess = IoThreadToProcess(TargetThread);
    HANDLE TargetPID = PsGetProcessId(TargetProcess);
    HANDLE CurrentPID = PsGetCurrentProcessId();

    if (TargetPID == CurrentPID || CurrentPID == (HANDLE)4 || !IsProcessProtected(TargetPID)) return OB_PREOP_SUCCESS;

    // Strip thread manipulation access rights
    if (OperationInformation->Operation == OB_OPERATION_HANDLE_CREATE) {
        if (OperationInformation->Parameters->CreateHandleInformation.OriginalDesiredAccess & PROTECT_THREAD_ACCESS) {
            OperationInformation->Parameters->CreateHandleInformation.DesiredAccess = 0;
        }
    }
    return OB_PREOP_SUCCESS;
}

/**
 * @brief Verifies if a process is in the protected whitelist.
 */
BOOLEAN IsProcessProtected(HANDLE ProcessId)
{
    BOOLEAN protected = FALSE;
    ExAcquireFastMutex(&g_ProtectionMutex);
    for (ULONG i = 0; i < g_ProtectedCount; i++) {
        if (g_Process_Protected[i].ProcessId == ProcessId) {
            protected = TRUE; break;
        }
    }
    ExReleaseFastMutex(&g_ProtectionMutex);
    return protected;
}

/**
 * @brief Records security threat events into a circular queue.
 */
VOID AddSecurityAlert(HANDLE AttackerPID, HANDLE TargetPID, PCHAR AttackerName, ACCESS_MASK Access)
{
    KIRQL oldIrql;
    KeAcquireSpinLock(&g_AlertLock, &oldIrql);

    PSECURITY_ALERT alert = &g_AlertQueue[g_AlertIndex % MAX_ALERTS];
    alert->AttackerPID = AttackerPID;
    alert->TargetPID = TargetPID;
    RtlStringCbCopyA(alert->AttackerName, sizeof(alert->AttackerName), AttackerName);
    KeQuerySystemTime(&alert->Timestamp);
    alert->AttemptedAccess = Access;

    g_AlertIndex++;
    KeReleaseSpinLock(&g_AlertLock, oldIrql);
}

/**
 * @brief Unregisters protection callbacks and clears the protected process list.
 */
NTSTATUS DisableProcessProtection(VOID)
{
    if (!g_ProtectionEnabled || !g_ObCallbackHandle) return STATUS_NOT_FOUND;

    StopIntegrityCheck();
    ObUnRegisterCallbacks(g_ObCallbackHandle);
    
    g_ObCallbackHandle = NULL;
    g_ProtectionEnabled = FALSE;

    ExAcquireFastMutex(&g_ProtectionMutex);
    RtlZeroMemory(g_Process_Protected, sizeof(g_Process_Protected));
    g_ProtectedCount = 0;
    ExReleaseFastMutex(&g_ProtectionMutex);

    return STATUS_SUCCESS;
}

/**
 * @brief Adds a process to the protected whitelist and registers CRC tracking.
 */
NTSTATUS AddProtectedProcess(HANDLE ProcessId)
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS Process = NULL;

    ExAcquireFastMutex(&g_ProtectionMutex);
    
    // Check duplication and limit
    for (ULONG i = 0; i < g_ProtectedCount; i++) {
        if (g_Process_Protected[i].ProcessId == ProcessId) {
            status = STATUS_ALREADY_REGISTERED; goto Exit;
        }
    }

    if (g_ProtectedCount >= 10) { status = STATUS_INSUFFICIENT_RESOURCES; goto Exit; }

    status = PsLookupProcessByProcessId(ProcessId, &Process);
    if (NT_SUCCESS(status)) {
        g_Process_Protected[g_ProtectedCount].ProcessId = ProcessId;
        g_Process_Protected[g_ProtectedCount].IsValid = TRUE;
        g_ProtectedCount++;
        AddCRC(ProcessId);
        ObDereferenceObject(Process);
    }

Exit:
    ExReleaseFastMutex(&g_ProtectionMutex);
    return status;
}

/**
 * @brief Transfers security alerts to user-mode via output buffer.
 */
NTSTATUS GetAlert_Queue(PVOID outputBuffer, ULONG outputLength, PULONG bytesTransferred)
{
    if (outputLength < sizeof(g_AlertQueue)) return STATUS_BUFFER_TOO_SMALL;

    KIRQL oldIrql;
    KeAcquireSpinLock(&g_AlertLock, &oldIrql);

    if (g_AlertIndex == 0) {
        KeReleaseSpinLock(&g_AlertLock, oldIrql);
        return STATUS_NO_MORE_ENTRIES;
    }

    RtlCopyMemory(outputBuffer, g_AlertQueue, sizeof(g_AlertQueue));
    *bytesTransferred = sizeof(g_AlertQueue);
    RtlZeroMemory(g_AlertQueue, sizeof(g_AlertQueue));
    g_AlertIndex = 0;

    KeReleaseSpinLock(&g_AlertLock, oldIrql);
    return STATUS_SUCCESS;
}
