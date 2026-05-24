/**
 * @file GetHardware.c
 * @brief Implementation of hardware information gathering.
 * @details SMBIOS table parsing to extract motherboard UUID and CPU ID.
 */

#include "GetHardware.h"
#include "../Context/HardWareContext/HardWareContext.h"
#include "../Context/HwSecurityProtocol/HwSecurityProtocol.h"
#include <ntstrsafe.h>

/**
 * @brief Get motherboard serial number (UUID).
 * @details Extract UUID from SMBIOS Type 1 (System Information).
 */
NTSTATUS GetMainboardSerial(PCHAR Buffer, ULONG BufferSize)
{
    PHYSICAL_ADDRESS physicalAddress;
    PVOID mappedAddress;
    PSMBIOS_ENTRY_POINT entryPoint = NULL;
    PVOID smbiosTable = NULL;
    PUCHAR tablePtr;
    PUCHAR tableEnd;
    NTSTATUS status = STATUS_NOT_FOUND;

    if (!Buffer || BufferSize == 0) {
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(Buffer, BufferSize);

    // Search for SMBIOS entry point (0xF0000 ~ 0xFFFFF range)
    physicalAddress.QuadPart = 0xF0000;
    mappedAddress = MmMapIoSpace(physicalAddress, 0x10000, MmNonCached);
    if (!mappedAddress) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Search for "_SM_" signature (16-byte alignment)
    for (ULONG offset = 0; offset < 0x10000 - sizeof(SMBIOS_ENTRY_POINT); offset += 16) {
        PSMBIOS_ENTRY_POINT candidate = (PSMBIOS_ENTRY_POINT)((PUCHAR)mappedAddress + offset);
        if (RtlCompareMemory(candidate->Anchor, "_SM_", 4) == 4) {
            // Checksum verification
            UCHAR checksum = 0;
            PUCHAR ptr = (PUCHAR)candidate;
            for (ULONG i = 0; i < candidate->Length; i++) {
                checksum += ptr[i];
            }
            if (checksum == 0) {
                entryPoint = candidate;
                break;
            }
        }
    }

    if (!entryPoint) {
        MmUnmapIoSpace(mappedAddress, 0x10000);
        return STATUS_NOT_FOUND;
    }

    // Map SMBIOS table
    PHYSICAL_ADDRESS tablePhysAddr;
    tablePhysAddr.QuadPart = entryPoint->TableAddress;
    ULONG tableSize = entryPoint->TableLength;
    MmUnmapIoSpace(mappedAddress, 0x10000);

    smbiosTable = MmMapIoSpace(tablePhysAddr, tableSize, MmNonCached);
    if (!smbiosTable) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    tablePtr = (PUCHAR)smbiosTable;
    tableEnd = tablePtr + tableSize;

    // Search for Type 1 (System Information)
    while (tablePtr < tableEnd) {
        PSMBIOS_HEADER header = (PSMBIOS_HEADER)tablePtr;
        if (header->Type == 1) {
            PSMBIOS_SYSTEM_INFO sysInfo = (PSMBIOS_SYSTEM_INFO)header;

            // Convert UUID to hex string (32 characters)
            RtlStringCchPrintfA(Buffer, BufferSize,
                "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                sysInfo->UUID[0], sysInfo->UUID[1], sysInfo->UUID[2], sysInfo->UUID[3],
                sysInfo->UUID[4], sysInfo->UUID[5],
                sysInfo->UUID[6], sysInfo->UUID[7],
                sysInfo->UUID[8], sysInfo->UUID[9],
                sysInfo->UUID[10], sysInfo->UUID[11], sysInfo->UUID[12], sysInfo->UUID[13],
                sysInfo->UUID[14], sysInfo->UUID[15]);

            status = STATUS_SUCCESS;
            break;
        }

        // Move to next structure
        tablePtr += header->Length;
        while (tablePtr < tableEnd && (tablePtr[0] != 0 || tablePtr[1] != 0)) {
            tablePtr++;
        }
        tablePtr += 2;
    }

    // If UUID not found, set to NONE
    if (status != STATUS_SUCCESS) {
        RtlStringCbCopyA(Buffer, BufferSize, "NONE");
        status = STATUS_SUCCESS;
    }

    MmUnmapIoSpace(smbiosTable, tableSize);
    return status;
}

/**
 * @brief Get CPU information.
 * @details Extract ProcessorID from SMBIOS Type 4 (Processor) with deduplication.
 */
NTSTATUS GetCPUInfo(PCHAR Buffer, ULONG BufferSize)
{
    PHYSICAL_ADDRESS physicalAddress;
    PVOID mappedAddress;
    PSMBIOS_ENTRY_POINT entryPoint = NULL;
    PVOID smbiosTable = NULL;
    PUCHAR tablePtr;
    PUCHAR tableEnd;
    NTSTATUS status = STATUS_NOT_FOUND;
    ULONG currentPos = 0;
    CHAR foundCPUs[8][17]; // Storage for up to 8 unique CPU IDs
    ULONG uniqueCount = 0;

    if (!Buffer || BufferSize == 0) {
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(Buffer, BufferSize);
    RtlZeroMemory(foundCPUs, sizeof(foundCPUs));

    // Search for SMBIOS entry point
    physicalAddress.QuadPart = 0xF0000;
    mappedAddress = MmMapIoSpace(physicalAddress, 0x10000, MmNonCached);
    if (!mappedAddress) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Search for "_SM_" signature
    for (ULONG offset = 0; offset < 0x10000 - sizeof(SMBIOS_ENTRY_POINT); offset += 16) {
        PSMBIOS_ENTRY_POINT candidate = (PSMBIOS_ENTRY_POINT)((PUCHAR)mappedAddress + offset);
        if (RtlCompareMemory(candidate->Anchor, "_SM_", 4) == 4) {
            UCHAR checksum = 0;
            PUCHAR ptr = (PUCHAR)candidate;
            for (ULONG i = 0; i < candidate->Length; i++) {
                checksum += ptr[i];
            }
            if (checksum == 0) {
                entryPoint = candidate;
                break;
            }
        }
    }

    if (!entryPoint) {
        MmUnmapIoSpace(mappedAddress, 0x10000);
        return STATUS_NOT_FOUND;
    }

    // Map SMBIOS table
    PHYSICAL_ADDRESS tablePhysAddr;
    tablePhysAddr.QuadPart = entryPoint->TableAddress;
    ULONG tableSize = entryPoint->TableLength;
    MmUnmapIoSpace(mappedAddress, 0x10000);

    smbiosTable = MmMapIoSpace(tablePhysAddr, tableSize, MmNonCached);
    if (!smbiosTable) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    tablePtr = (PUCHAR)smbiosTable;
    tableEnd = tablePtr + tableSize;

    // Traverse all Type 4 (Processor) structures
    while (tablePtr < tableEnd) {
        PSMBIOS_HEADER header = (PSMBIOS_HEADER)tablePtr;

        if (header->Type == 4) {
            PSMBIOS_PROCESSOR_INFO procInfo = (PSMBIOS_PROCESSOR_INFO)header;

            // Convert ProcessorID to hex string
            CHAR cpuId[17];
            RtlStringCchPrintfA(cpuId, sizeof(cpuId),
                "%02X%02X%02X%02X%02X%02X%02X%02X",
                procInfo->ProcessorID[7], procInfo->ProcessorID[6],
                procInfo->ProcessorID[5], procInfo->ProcessorID[4],
                procInfo->ProcessorID[3], procInfo->ProcessorID[2],
                procInfo->ProcessorID[1], procInfo->ProcessorID[0]);

            // Deduplication check
            BOOLEAN isDuplicate = FALSE;
            for (ULONG i = 0; i < uniqueCount; i++) {
                if (strcmp(foundCPUs[i], cpuId) == 0) {
                    isDuplicate = TRUE;
                    break;
                }
            }

            // Save if unique
            if (!isDuplicate && uniqueCount < 8) {
                RtlStringCchCopyA(foundCPUs[uniqueCount], sizeof(foundCPUs[0]), cpuId);
                uniqueCount++;
            }
        }

        // Move to next structure
        tablePtr += header->Length;
        while (tablePtr < tableEnd && (tablePtr[0] != 0 || tablePtr[1] != 0)) {
            tablePtr++;
        }
        tablePtr += 2;
    }

    // Copy deduplicated CPU IDs to buffer
    for (ULONG i = 0; i < uniqueCount; i++) {
        if (i > 0 && currentPos < BufferSize - 3) {
            Buffer[currentPos++] = ',';
            Buffer[currentPos++] = ' ';
        }

        ULONG len = 16;
        if (currentPos + len < BufferSize) {
            RtlCopyMemory(Buffer + currentPos, foundCPUs[i], len);
            currentPos += len;
            status = STATUS_SUCCESS;
        }
    }

    // If no CPUs found
    if (uniqueCount == 0) {
        RtlStringCbCopyA(Buffer, BufferSize, "NONE");
        status = STATUS_SUCCESS;
    }

    MmUnmapIoSpace(smbiosTable, tableSize);
    return status;
}

/**
 * @brief Get SMBIOS string.
 * @details SMBIOS strings are stored consecutively after the structure, null-terminated.
 */
PCHAR GetSMBIOSString(PUCHAR StringPtr, UCHAR StringIndex)
{
    UCHAR currentIndex = 1;

    if (StringIndex == 0) {
        return NULL;
    }

    while (*StringPtr != 0) {
        if (currentIndex == StringIndex) {
            return (PCHAR)StringPtr;
        }

        while (*StringPtr != 0) {
            StringPtr++;
        }
        StringPtr++;
        currentIndex++;
    }

    return NULL;
}

/**
 * @brief Build hardware information response message (TLV structure).
 */
NTSTATUS BuildHardwareResponseMessage(ULONG RequestType, PUCHAR* Message, PULONG MessageSize)
{
    CHAR mainboardSerial[256] = { 0 };
    CHAR cpuInfo[256] = { 0 };
    CHAR memoryInfo[256] = { 0 };
    ULONG totalSize = sizeof(MESSAGE_HEADER);
    PUCHAR buffer = NULL;
    PUCHAR currentPtr = NULL;
    ULONG fieldCount = 0;

    // Collect info based on request type
    switch (RequestType) {
    case HW_REQUEST_ALL:
        GetMainboardSerial(mainboardSerial, sizeof(mainboardSerial));
        GetCPUInfo(cpuInfo, sizeof(cpuInfo));
        totalSize += sizeof(MESSAGE_FIELD) - 1 + (ULONG)strlen(mainboardSerial);
        totalSize += sizeof(MESSAGE_FIELD) - 1 + (ULONG)strlen(cpuInfo);
        fieldCount = 2;
        break;

    case HW_REQUEST_MAINBOARD:
        GetMainboardSerial(mainboardSerial, sizeof(mainboardSerial));
        totalSize += sizeof(MESSAGE_FIELD) - 1 + (ULONG)strlen(mainboardSerial);
        fieldCount = 1;
        break;

    case HW_REQUEST_CPU:
        GetCPUInfo(cpuInfo, sizeof(cpuInfo));
        totalSize += sizeof(MESSAGE_FIELD) - 1 + (ULONG)strlen(cpuInfo);
        fieldCount = 1;
        break;

    default:
        return STATUS_INVALID_PARAMETER;
    }

    // Allocate response buffer
    buffer = (PUCHAR)ExAllocatePoolWithTag(NonPagedPool, totalSize, 'wdHW');
    if (!buffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(buffer, totalSize);
    currentPtr = buffer;

    // Write message header
    static LONG g_MessageIdCounter = 0;
    PMESSAGE_HEADER header = (PMESSAGE_HEADER)currentPtr;
    header->MessageType = MSG_TYPE_HARDWARE_RESPONSE;
    header->MessageId = (ULONG)InterlockedIncrement(&g_MessageIdCounter);
    header->FieldCount = (USHORT)fieldCount;
    header->Reserved = 0;
    currentPtr += sizeof(MESSAGE_HEADER);

    // Add motherboard field
    if (RequestType == HW_REQUEST_ALL || RequestType == HW_REQUEST_MAINBOARD) {
        PMESSAGE_FIELD field = (PMESSAGE_FIELD)currentPtr;
        field->FieldId = HW_REQUEST_MAINBOARD;
        field->DataSize = (ULONG)strlen(mainboardSerial);
        RtlCopyMemory(field->Data, mainboardSerial, field->DataSize);
        currentPtr += sizeof(MESSAGE_FIELD) - 1 + field->DataSize;
    }

    // Add CPU field
    if (RequestType == HW_REQUEST_ALL || RequestType == HW_REQUEST_CPU) {
        PMESSAGE_FIELD field = (PMESSAGE_FIELD)currentPtr;
        field->FieldId = HW_REQUEST_CPU;
        field->DataSize = (ULONG)strlen(cpuInfo);
        RtlCopyMemory(field->Data, cpuInfo, field->DataSize);
        currentPtr += sizeof(MESSAGE_FIELD) - 1 + field->DataSize;
    }

    *Message = buffer;
    *MessageSize = totalSize;

    return STATUS_SUCCESS;
}
