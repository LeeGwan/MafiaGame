/**
 * @file crc.c
 * @brief CRC32-based process code integrity verification implementation.
 * @details Segments .text and .rdata sections into 256-byte chunks to generate CRC32 hashes
 * and periodically re-verifies memory to detect code tampering.
 */

#include "crc.h"
#include <Ntstrsafe.h>

// --- Global Variables ---
ULONG g_SelfFunctionSize = 0x1000;
ULONG CRC32Table[256] = { 0 };
BOOLEAN CRC32Initialized = FALSE;
PROCESS_CODE_HASH g_process_code_hash[10];
ULONG g_process_code_hashCount = 0;
BOOLEAN g_IntegrityCheckRunning = FALSE;
PKTHREAD g_IntegrityCheckThread = NULL;

/**
 * @brief Initialize CRC32 lookup table using the 0xEDB88320 polynomial.
 */
VOID InitializeCRC32Table(VOID)
{
    if (CRC32Initialized) {
        return;
    }

    for (ULONG i = 0; i < 256; i++) {
        ULONG crc = i;
        for (ULONG j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            }
            else {
                crc >>= 1;
            }
        }
        CRC32Table[i] = crc;
    }

    CRC32Initialized = TRUE;
}

/**
 * @brief Calculate CRC32 hash.
 */
ULONG CalculateCRC32(PUCHAR Data, ULONG Length)
{
    if (!CRC32Initialized) {
        InitializeCRC32Table();
    }

    ULONG crc = 0xFFFFFFFF;

    for (ULONG i = 0; i < Length; i++) {
        UCHAR byte = Data[i];
        ULONG index = (crc ^ byte) & 0xFF;
        crc = (crc >> 8) ^ CRC32Table[index];
    }

    return ~crc;
}

/**
 * @brief Read process memory using MmCopyVirtualMemory.
 */
NTSTATUS ReadProcessMemory(PEPROCESS Process, PVOID Address, PVOID Buffer, SIZE_T Size)
{
    NTSTATUS status;
    SIZE_T bytesRead = 0;

    if (!Process || !Address || !Buffer || Size == 0) {
        return STATUS_INVALID_PARAMETER;
    }

    // Read memory from target process
    status = MmCopyVirtualMemory(
        Process,
        Address,
        PsGetCurrentProcess(),
        Buffer,
        Size,
        KernelMode,
        &bytesRead
    );

    if (!NT_SUCCESS(status)) {
        DbgPrint("MmCopyVirtualMemory failed: 0x%08X (bytes read: %zu)\n",
            status, bytesRead);
        return status;
    }

    if (bytesRead != Size) {
        DbgPrint("Partial read: requested %zu, got %zu\n",
            Size, bytesRead);
        return STATUS_PARTIAL_COPY;
    }

    return STATUS_SUCCESS;
}

/**
 * @brief Get the base address of the process main module via PEB->ImageBaseAddress.
 */
PVOID GetProcessMainModuleBase(PEPROCESS Process)
{
    KAPC_STATE apcState;
    PVOID imageBase = NULL;

    if (!Process) {
        return NULL;
    }

    __try {
        PPEB peb = PsGetProcessPeb(Process);
        if (!peb) {
            return NULL;
        }

        // Switch to process context
        KeStackAttachProcess((PKPROCESS)Process, &apcState);

        if (MmIsAddressValid(peb)) {
            // ImageBaseAddress is located at offset 0x10 in PEB
            PVOID* pImageBase = (PVOID*)((PUCHAR)peb + 0x10);
            if (MmIsAddressValid(pImageBase)) {
                imageBase = *pImageBase;
            }
        }

        KeUnstackDetachProcess(&apcState);
        return imageBase;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return NULL;
    }
}

/**
 * @brief Find .text and .rdata sections of the PE file.
 */
ULONG FindCodeSections(PEPROCESS Process, PVOID ImageBase, CODE_SECTION_INFO Sections[2])
{
    KAPC_STATE apcState;
    ULONG foundCount = 0;

    if (!Process || !ImageBase || !Sections) {
        return 0;
    }

    Sections[0].Address = NULL;
    Sections[0].Size = 0;
    Sections[1].Address = NULL;
    Sections[1].Size = 0;

    __try {
        KeStackAttachProcess((PKPROCESS)Process, &apcState);

        if (!MmIsAddressValid(ImageBase)) {
            goto Exit;
        }

        // Verify DOS header
        PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)ImageBase;
        if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
            DbgPrint("Invalid DOS signature\n");
            goto Exit;
        }

        // Verify NT header
        PIMAGE_NT_HEADERS64 ntHeaders = (PIMAGE_NT_HEADERS64)((PUCHAR)ImageBase + dosHeader->e_lfanew);
        if (!MmIsAddressValid(ntHeaders)) {
            goto Exit;
        }

        if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
            DbgPrint("Invalid PE signature\n");
            goto Exit;
        }

        // Section header array start address
        PIMAGE_SECTION_HEADER sectionHeader = (PIMAGE_SECTION_HEADER)(
            (PUCHAR)ntHeaders + sizeof(IMAGE_NT_HEADERS64)
            );

        USHORT numberOfSections = ntHeaders->FileHeader.NumberOfSections;

        // Traverse all sections to find .text and .rdata
        for (USHORT i = 0; i < numberOfSections && foundCount < 2; i++) {
            if (!MmIsAddressValid(&sectionHeader[i])) {
                break;
            }

            BOOLEAN isText = RtlCompareMemory(sectionHeader[i].Name, ".text\0\0\0", 8) == 8;
            BOOLEAN isRdata = RtlCompareMemory(sectionHeader[i].Name, ".rdata\0\0", 8) == 8;

            if (isText || isRdata) {
                Sections[foundCount].Address = (PUCHAR)ImageBase + sectionHeader[i].VirtualAddress;
                Sections[foundCount].Size = sectionHeader[i].Misc.VirtualSize;

                DbgPrint("Found %s section: RVA=0x%X, Size=0x%X, Address=0x%p\n",
                    isText ? ".text" : ".rdata",
                    sectionHeader[i].VirtualAddress,
                    Sections[foundCount].Size,
                    Sections[foundCount].Address);

                foundCount++;
            }
        }

    Exit:
        KeUnstackDetachProcess(&apcState);
        return foundCount;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

/**
 * @brief Generate process code hash (256-byte chunks).
 */
NTSTATUS AddCRC(HANDLE ProcessId)
{
    NTSTATUS status;
    PEPROCESS process = NULL;
    PUCHAR codeBuffer = NULL;
    PVOID imageBase = NULL;
    CODE_SECTION_INFO sections[2];
    ULONG totalChunkCount = 0;

    // Lookup process object
    status = PsLookupProcessByProcessId(ProcessId, &process);
    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed to lookup process %d: 0x%08X\n",
            HandleToULong(ProcessId), status);
        return status;
    }

    // Get main module base address
    imageBase = GetProcessMainModuleBase(process);
    if (!imageBase) {
        DbgPrint("Failed to get base address for PID %d\n",
            HandleToULong(ProcessId));
        ObDereferenceObject(process);
        return STATUS_UNSUCCESSFUL;
    }

    // Find .text and .rdata sections
    ULONG sectionCount = FindCodeSections(process, imageBase, sections);
    if (sectionCount == 0) {
        DbgPrint("Failed to find code sections for PID %d\n",
            HandleToULong(ProcessId));
        ObDereferenceObject(process);
        return STATUS_UNSUCCESSFUL;
    }

    // Calculate total number of chunks
    for (ULONG s = 0; s < sectionCount; s++) {
        totalChunkCount += (sections[s].Size + CODE_CHUNK_SIZE - 1) / CODE_CHUNK_SIZE;
    }

    // Allocate hash array
    PULONG hashValues = (PULONG)ExAllocatePoolWithTag(
        NonPagedPool,
        totalChunkCount * sizeof(ULONG),
        'hshC'
    );

    if (!hashValues) {
        DbgPrint("Failed to allocate hash array for %d chunks\n", totalChunkCount);
        ObDereferenceObject(process);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(hashValues, totalChunkCount * sizeof(ULONG));

    DbgPrint("PID %d: total chunks %d\n",
        HandleToULong(ProcessId), totalChunkCount);

    g_process_code_hash[g_process_code_hashCount].ProcessId = ProcessId;

    // Allocate buffer for reading code
    codeBuffer = (PUCHAR)ExAllocatePoolWithTag(NonPagedPool, CODE_CHUNK_SIZE, 'hCIC');
    if (!codeBuffer) {
        ExFreePool(hashValues);
        ObDereferenceObject(process);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Calculate CRC32 for each section, split into 256-byte chunks
    ULONG hashIndex = 0;
    for (ULONG s = 0; s < sectionCount; s++) {
        ULONG sectionSize = sections[s].Size;
        PVOID sectionAddress = sections[s].Address;
        ULONG chunkCount = (sectionSize + CODE_CHUNK_SIZE - 1) / CODE_CHUNK_SIZE;

        for (ULONG i = 0; i < chunkCount; i++) {
            ULONG offset = i * CODE_CHUNK_SIZE;
            ULONG currentChunkSize = CODE_CHUNK_SIZE;

            // Last chunk may be smaller
            if (offset + CODE_CHUNK_SIZE > sectionSize) {
                currentChunkSize = sectionSize - offset;
            }

            RtlZeroMemory(codeBuffer, CODE_CHUNK_SIZE);

            PVOID chunkAddress = (PUCHAR)sectionAddress + offset;
            status = ReadProcessMemory(process, chunkAddress, codeBuffer, currentChunkSize);

            if (!NT_SUCCESS(status)) {
                DbgPrint("Failed to read chunk %d for PID %d: 0x%08X\n",
                    hashIndex, HandleToULong(ProcessId), status);
                ExFreePool(hashValues);
                ExFreePool(codeBuffer);
                ObDereferenceObject(process);
                return status;
            }

            // Calculate CRC32 hash
            hashValues[hashIndex] = CalculateCRC32(codeBuffer, currentChunkSize);
            hashIndex++;
        }
    }

    // Save to global hash array
    g_process_code_hash[g_process_code_hashCount].CodeBaseAddress = sections[0].Address;
    g_process_code_hash[g_process_code_hashCount].CodeSize = 0;
    for (ULONG s = 0; s < sectionCount; s++) {
        g_process_code_hash[g_process_code_hashCount].CodeSize += sections[s].Size;
    }
    g_process_code_hash[g_process_code_hashCount].ChunkCount = totalChunkCount;
    g_process_code_hash[g_process_code_hashCount].HashValues = hashValues;
    g_process_code_hash[g_process_code_hashCount].SectionCount = sectionCount;
    for (ULONG s = 0; s < sectionCount; s++) {
        g_process_code_hash[g_process_code_hashCount].Sections[s] = sections[s];
    }
    g_process_code_hash[g_process_code_hashCount].IsValid = TRUE;
    g_process_code_hashCount++;

    DbgPrint("Computed all hashes for PID %d successfully\n", HandleToULong(ProcessId));

    ExFreePool(codeBuffer);
    ObDereferenceObject(process);

    return STATUS_SUCCESS;
}

/**
 * @brief Verify process code hash (detect memory tampering).
 */
BOOLEAN ComputeProcessCodeHash(PPROCESS_CODE_HASH hashInfo)
{
    NTSTATUS status;
    PEPROCESS process = NULL;
    PUCHAR codeBuffer = NULL;
    BOOLEAN result = TRUE;

    if (!hashInfo) {
        DbgPrint("No hash info found\n");
        return FALSE;
    }

    status = PsLookupProcessByProcessId(hashInfo->ProcessId, &process);
    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed to lookup process %d: 0x%08X\n",
            HandleToULong(hashInfo->ProcessId), status);
        return FALSE;
    }

    codeBuffer = (PUCHAR)ExAllocatePoolWithTag(NonPagedPool, CODE_CHUNK_SIZE, 'hCIC');
    if (!codeBuffer) {
        ObDereferenceObject(process);
        return FALSE;
    }

    // Re-read each chunk, re-calculate CRC32, and compare
    ULONG hashIndex = 0;
    for (ULONG s = 0; s < hashInfo->SectionCount; s++) {
        ULONG sectionSize = hashInfo->Sections[s].Size;
        PVOID sectionAddress = hashInfo->Sections[s].Address;
        ULONG chunkCount = (sectionSize + CODE_CHUNK_SIZE - 1) / CODE_CHUNK_SIZE;

        for (ULONG i = 0; i < chunkCount; i++) {
            ULONG offset = i * CODE_CHUNK_SIZE;
            ULONG currentChunkSize = CODE_CHUNK_SIZE;

            if (offset + CODE_CHUNK_SIZE > sectionSize) {
                currentChunkSize = sectionSize - offset;
            }

            RtlZeroMemory(codeBuffer, CODE_CHUNK_SIZE);

            PVOID chunkAddress = (PUCHAR)sectionAddress + offset;
            status = ReadProcessMemory(process, chunkAddress, codeBuffer, currentChunkSize);

            if (!NT_SUCCESS(status)) {
                result = FALSE;
                break;
            }

            // Calculate current CRC32
            ULONG currentHash = CalculateCRC32(codeBuffer, currentChunkSize);

            // Compare with original hash
            if (currentHash != hashInfo->HashValues[hashIndex]) {
                DbgPrint("[MemoryChanged]\n");
                DbgPrint("========================================================\n");
                DbgPrint("Section %d, Offset: 0x%X, Size: 0x%X\n", s, offset, currentChunkSize);
                DbgPrint("[ComputeProcessCodeHash] Original: 0x%08X, Current: 0x%08X\n",
                    hashInfo->HashValues[hashIndex], currentHash);
                DbgPrint("========================================================\n");
                result = FALSE;
                break;
            }

            hashIndex++;
        }

        if (!result) break;
    }

    if (result) {
        DbgPrint("Verification passed for PID %d (all %d chunks OK)\n",
            HandleToULong(hashInfo->ProcessId), hashInfo->ChunkCount);
    }

    ExFreePool(codeBuffer);
    ObDereferenceObject(process);

    return result;
}

/**
 * @brief Initialize the integrity check system.
 */
NTSTATUS InitializeIntegrityCheck(VOID)
{
    NTSTATUS status;
    HANDLE threadHandle = NULL;
    OBJECT_ATTRIBUTES objAttr;

    if (g_IntegrityCheckRunning) {
        return STATUS_SUCCESS;
    }

    RtlZeroMemory(g_process_code_hash, sizeof(g_process_code_hash));
    g_IntegrityCheckRunning = TRUE;

    InitializeCRC32Table();

    InitializeObjectAttributes(&objAttr, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

    // Create system thread for integrity check
    status = PsCreateSystemThread(
        &threadHandle,
        THREAD_ALL_ACCESS,
        &objAttr,
        NULL,
        NULL,
        IntegrityCheckThreadRoutine,
        NULL
    );

    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed to create integrity check thread: 0x%08X\n", status);
        g_IntegrityCheckRunning = FALSE;
        return status;
    }

    // Reference the thread object
    status = ObReferenceObjectByHandle(
        threadHandle,
        THREAD_ALL_ACCESS,
        *PsThreadType,
        KernelMode,
        (PVOID*)&g_IntegrityCheckThread,
        NULL
    );

    ZwClose(threadHandle);

    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed to reference thread object: 0x%08X\n", status);
        g_IntegrityCheckRunning = FALSE;
        return status;
    }

    DbgPrint(" Integrity check system initialized\n");
    return STATUS_SUCCESS;
}

/**
 * @brief Shutdown the integrity check system.
 */
VOID StopIntegrityCheck(VOID)
{
    if (!g_IntegrityCheckRunning) {
        return;
    }

    g_IntegrityCheckRunning = FALSE;

    // Wait for thread termination
    if (g_IntegrityCheckThread) {
        KeWaitForSingleObject(g_IntegrityCheckThread, Executive, KernelMode, FALSE, NULL);
        ObDereferenceObject(g_IntegrityCheckThread);
        g_IntegrityCheckThread = NULL;
    }

    // Free hash arrays
    for (ULONG i = 0; i < 10; i++) {
        if (g_process_code_hash[i].HashValues != NULL) {
            ExFreePool(g_process_code_hash[i].HashValues);
            g_process_code_hash[i].HashValues = NULL;
        }
    }

    RtlZeroMemory(g_process_code_hash, sizeof(g_process_code_hash));
}

/**
 * @brief Integrity check thread routine (verifies every 10 seconds).
 */
VOID IntegrityCheckThreadRoutine(PVOID Context)
{
    UNREFERENCED_PARAMETER(Context);

    while (g_IntegrityCheckRunning) {
        // Verify all registered processes
        if (g_process_code_hashCount > 0) {
            for (ULONG i = 0; i < 10; i++) {
                if (g_process_code_hash[i].IsValid && g_process_code_hash[i].ProcessId != NULL) {
                    BOOLEAN result = ComputeProcessCodeHash(&g_process_code_hash[i]);

                    if (!result) {
                        // Additional action possible upon memory tampering detection
                    }
                }
            }
        }

        // Wait 10 seconds
        LARGE_INTEGER interval;
        interval.QuadPart = -100000000LL;  // 10 seconds (100ns units)
        KeDelayExecutionThread(KernelMode, FALSE, &interval);
    }

    DbgPrint("Integrity check thread stopped\n");
    PsTerminateSystemThread(STATUS_SUCCESS);
}
