/**
 * @file crc.h
 * @brief Header for CRC32-based process code integrity verification.
 * @details Manages code section analysis (.text, .rdata) and background memory monitoring.
 */

#pragma once
#include <ntddk.h>
#include "../Context/ProcessCodeHash/ProcessCodeHash.h"

// --- Global Variables ---

extern ULONG g_SelfFunctionSize;                  /**< Function size (reserved). */
extern PROCESS_CODE_HASH g_process_code_hash[10]; /**< Storage for code hashes of protected processes. */
extern ULONG g_process_code_hashCount;            /**< Number of currently monitored processes. */
extern ULONG CRC32Table[256];                     /**< Lookup table for fast CRC32 calculation. */
extern BOOLEAN CRC32Initialized;                  /**< Initialization flag for CRC32 table. */
extern BOOLEAN g_IntegrityCheckRunning;           /**< Flag for background integrity thread status. */
extern PKTHREAD g_IntegrityCheckThread;           /**< Thread object for the integrity monitor. */

// --- Undocumented/Internal API Declarations ---

/** @brief Attaches the current thread to the address space of the target process. */
NTKERNELAPI
VOID
KeStackAttachProcess(
    _In_ PKPROCESS Process,
    _Out_ PKAPC_STATE ApcState
);

/** @brief Detaches the current thread from the address space of a process. */
NTKERNELAPI
VOID
KeUnstackDetachProcess(
    _In_ PKAPC_STATE ApcState
);

/** @brief Retrieves the PEB (Process Environment Block) pointer for a process. */
NTKERNELAPI
PPEB
PsGetProcessPeb(
    _In_ PEPROCESS Process
);

/** @brief Copies virtual memory from a source process to the current process. */
NTKERNELAPI
NTSTATUS
MmCopyVirtualMemory(
    _In_ PEPROCESS SourceProcess,
    _In_ PVOID SourceAddress,
    _In_ PEPROCESS TargetProcess,
    _Out_ PVOID TargetAddress,
    _In_ SIZE_T BufferSize,
    _In_ KPROCESSOR_MODE PreviousMode,
    _Out_ PSIZE_T ReturnSize
);

// --- Integrity Verification System Lifecycle ---

/** @brief Initializes the integrity monitoring system and spawns a monitor thread. */
NTSTATUS InitializeIntegrityCheck(VOID);

/** @brief Stops the integrity monitoring thread and releases allocated resources. */
VOID StopIntegrityCheck(VOID);

// --- CRC32 Calculation Utilities ---

/** @brief Initializes the CRC32 lookup table. */
VOID InitializeCRC32Table(VOID);

/** @brief Calculates a CRC32 checksum for a given memory buffer. */
ULONG CalculateCRC32(PUCHAR Data, ULONG Length);

// --- Memory Access Interface ---

/** @brief Reads raw memory from a target process safely. */
NTSTATUS ReadProcessMemory(PEPROCESS Process, PVOID Address, PVOID Buffer, SIZE_T Size);

// --- PE Parsing and Code Analysis ---

/** @brief Locates the base address of the main module (image base) for a given process. */
PVOID GetProcessMainModuleBase(PEPROCESS Process);

/** @brief Analyzes PE headers to identify executable sections (.text, .rdata). */
ULONG FindCodeSections(PEPROCESS Process, PVOID ImageBase, CODE_SECTION_INFO Sections[2]);

// --- Hash Generation and Monitoring ---

/** @brief Performs initial CRC32 hash calculation for process sections and saves state. */
NTSTATUS AddCRC(HANDLE ProcessId);

/** @brief Re-verifies process memory against stored CRC32 hashes. */
BOOLEAN ComputeProcessCodeHash(PPROCESS_CODE_HASH hashInfo);

/** @brief Background thread routine for periodic (10s) integrity verification. */
VOID IntegrityCheckThreadRoutine(PVOID Context);
