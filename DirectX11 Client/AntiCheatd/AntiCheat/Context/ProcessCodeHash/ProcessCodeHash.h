/**
 * @file Integrity.h
 * @brief Definitions for PE header parsing and CRC32-based memory integrity verification.
 * @details This header defines structures for scanning process code sections (.text, .rdata) 
 * and tracking security violations at the kernel level.
 */

#pragma once
#include <ntddk.h>

#define CODE_HASH_SIZE 0x1000  /**< Default buffer size for code hashing (4KB). */
#define CODE_CHUNK_SIZE 0x100  /**< Granularity for integrity checks (256 bytes per chunk). */

// --- Standard PE Header Definitions (64-bit) ---

/** @struct IMAGE_DOS_HEADER
 * @brief The legacy MZ header used to locate the NT header.
 */
typedef struct _IMAGE_DOS_HEADER {
    USHORT e_magic;      /**< Magic number "MZ" (0x5A4D). */
    USHORT e_cblp;
    USHORT e_cp;
    USHORT e_crlc;
    USHORT e_cparhdr;
    USHORT e_minalloc;
    USHORT e_maxalloc;
    USHORT e_ss;
    USHORT e_sp;
    USHORT e_csum;
    USHORT e_ip;
    USHORT e_cs;
    USHORT e_lfarlc;
    USHORT e_ovno;
    USHORT e_res[4];
    USHORT e_oemid;
    USHORT e_oeminfo;
    USHORT e_res2[10];
    LONG e_lfanew;       /**< File address of the new EXE (NT) header. */
} IMAGE_DOS_HEADER, * PIMAGE_DOS_HEADER;

/** @struct IMAGE_NT_HEADERS64
 * @brief The primary PE header containing file and optional headers.
 */
typedef struct _IMAGE_NT_HEADERS64 {
    ULONG Signature;                      /**< PE Signature "PE\0\0" (0x4550). */
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER64 OptionalHeader;
} IMAGE_NT_HEADERS64, * PIMAGE_NT_HEADERS64;

/** @struct IMAGE_SECTION_HEADER
 * @brief Metadata for executable sections (e.g., .text, .data).
 */
typedef struct _IMAGE_SECTION_HEADER {
    UCHAR Name[8];
    union {
        ULONG PhysicalAddress;
        ULONG VirtualSize;
    } Misc;
    ULONG VirtualAddress;                 /**< RVA (Relative Virtual Address). */
    ULONG SizeOfRawData;
    ULONG PointerToRawData;
    ULONG PointerToRelocations;
    ULONG PointerToLinenumbers;
    USHORT NumberOfRelocations;
    USHORT NumberOfLinenumbers;
    ULONG Characteristics;                /**< Flags (Execute, Read, Write). */
} IMAGE_SECTION_HEADER, * PIMAGE_SECTION_HEADER;

// --- Integrity Management Structures ---

/** @struct CODE_SECTION_INFO
 * @brief Metadata for individual executable sections to be monitored.
 */
typedef struct _CODE_SECTION_INFO {
    PVOID Address;  /**< Base address of the section in memory. */
    ULONG Size;     /**< Total size of the section. */
} CODE_SECTION_INFO, * PCODE_SECTION_INFO;

/** @struct PROCESS_CODE_HASH
 * @brief Centralized record for a process's code integrity state.
 * @details Used to compare current memory states against pre-calculated CRC32 hashes.
 */
typedef struct _PROCESS_CODE_HASH {
    HANDLE ProcessId;                /**< Target process identifier. */
    PVOID CodeBaseAddress;           /**< Primary module base address. */
    ULONG CodeSize;                  /**< Cumulative size of code to scan. */
    ULONG ChunkCount;                /**< Total number of 256-byte verification chunks. */
    PULONG HashValues;               /**< Dynamic array of golden CRC32 hashes. */
    ULONG SectionCount;              /**< Number of monitored sections (e.g., .text, .rdata). */
    CODE_SECTION_INFO Sections[2];   /**< Section metadata array. */
    BOOLEAN IsValid;                 /**< State flag indicating if tracking is active. */
} PROCESS_CODE_HASH, * PPROCESS_CODE_HASH;

/** @struct SECURITY_ALERT
 * @brief Detailed log of a blocked access or integrity violation.
 */
typedef struct _SECURITY_ALERT {
    HANDLE AttackerPID;              /**< PID of the process attempting unauthorized access. */
    HANDLE TargetPID;                /**< PID of the protected application. */
    CHAR AttackerName[16];           /**< Process name of the violator. */
    LARGE_INTEGER Timestamp;         /**< System time of the event. */
    ACCESS_MASK AttemptedAccess;     /**< Mask of the denied access rights. */
} SECURITY_ALERT, * PSECURITY_ALERT;

// --- Native Kernel Function Prototypes ---

/** @brief Retrieves the short image file name for a process. */
NTKERNELAPI PCHAR PsGetProcessImageFileName(_In_ PEPROCESS Process);

/** @brief Looks up a PEPROCESS object from a PID with a reference increment. */
NTKERNELAPI NTSTATUS PsLookupProcessByProcessId(_In_ HANDLE ProcessId, _Outptr_ PEPROCESS* Process);
