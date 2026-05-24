/**
 * @file ProcessGuard.h
 * @brief Kernel-mode process and thread protection interface.
 * @details Declarations for ObRegisterCallbacks-based handle access control 
 * and security event telemetry.
 */

#pragma once
#include <ntddk.h>
#include "../Context/ProcessCodeHash/ProcessCodeHash.h"

#define MAX_ALERTS 100 /**< Maximum capacity for the security violation log queue. */

// --- Global State Management ---
extern PVOID g_ObCallbackHandle;                 /**< Handle returned by ObRegisterCallbacks. */
extern BOOLEAN g_ProtectionEnabled;             /**< Flag: Protection active state. */
extern PROCESS_PROTECTED g_Process_Protected[10];/**< Whitelist of protected process IDs. */
extern ULONG g_ProtectedCount;                  /**< Current count of protected processes. */
extern FAST_MUTEX g_ProtectionMutex;            /**< Mutex for thread-safe whitelist access. */
extern KSPIN_LOCK g_AlertLock;                  /**< Spinlock for high-frequency security log updates. */
extern SECURITY_ALERT g_AlertQueue[MAX_ALERTS]; /**< Circular buffer for security violations. */
extern ULONG g_AlertIndex;                      /**< Current index in the alert queue. */

// --- ObRegisterCallback Handlers ---

/** @brief Intercepts handle operations (Create/Duplicate) before they occur. */
OB_PREOP_CALLBACK_STATUS PreOperationCallback(
    _In_ PVOID RegistrationContext,
    _In_ POB_PRE_OPERATION_INFORMATION OperationInformation
);

/** @brief Performs post-operation logging after a handle operation is completed. */
VOID PostOperationCallback(
    _In_ PVOID RegistrationContext,
    _In_ POB_POST_OPERATION_INFORMATION OperationInformation
);

/** @brief Intercepts thread-specific handle operations to prevent context hijacking. */
OB_PREOP_CALLBACK_STATUS ThreadPreOperationCallback(
    _In_ PVOID RegistrationContext,
    _In_ POB_PRE_OPERATION_INFORMATION OperationInformation
);

/** @brief Helper: Retrieves the PEPROCESS object from an ETHREAD structure. */
NTKERNELAPI PEPROCESS IoThreadToProcess(_In_ PETHREAD Thread);

// --- Security Orchestration API ---

/** @brief Registers the kernel callbacks to initiate global process protection. */
NTSTATUS EnableProcessProtection(VOID);

/** @brief Checks if a given PID is currently under protection. */
BOOLEAN IsProcessProtected(HANDLE ProcessId);

/** @brief Logs unauthorized access attempts for server telemetry. */
VOID AddSecurityAlert(HANDLE AttackerPID, HANDLE TargetPID, PCHAR AttackerName, ACCESS_MASK Access);

/** @brief Safely unregisters kernel callbacks and clears state. */
NTSTATUS DisableProcessProtection(VOID);

/** @brief Adds a process PID to the protected whitelist and triggers integrity monitoring. */
NTSTATUS AddProtectedProcess(HANDLE ProcessId);

/** @brief Retrieves the collected security alerts for transmission to the User-Mode application. */
NTSTATUS GetAlert_Queue(PVOID outputBuffer, ULONG outputLength, PULONG bytesTransferred);
