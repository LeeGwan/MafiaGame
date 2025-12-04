// 하드웨어 정보 수집 구현부
// SMBIOS 테이블 파싱을 통한 메인보드 UUID 및 CPU ID 추출
#include "GetHardware.h"
#include "../Context/HardWareContext/HardWareContext.h"
#include "../Context/HwSecurityProtocol/HwSecurityProtocol.h"
#include <ntstrsafe.h>

// 메인보드 시리얼 번호 (UUID) 가져오기
// SMBIOS Type 1 (System Information)에서 UUID 추출
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

    // SMBIOS 엔트리 포인트 검색 (0xF0000 ~ 0xFFFFF 영역)
    physicalAddress.QuadPart = 0xF0000;
    mappedAddress = MmMapIoSpace(physicalAddress, 0x10000, MmNonCached);
    if (!mappedAddress) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // "_SM_" 시그니처 검색 (16바이트 단위)
    for (ULONG offset = 0; offset < 0x10000 - sizeof(SMBIOS_ENTRY_POINT); offset += 16) {
        PSMBIOS_ENTRY_POINT candidate = (PSMBIOS_ENTRY_POINT)((PUCHAR)mappedAddress + offset);
        if (RtlCompareMemory(candidate->Anchor, "_SM_", 4) == 4) {
            // 체크섬 검증
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

    // SMBIOS 테이블 매핑
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

    // Type 1 (System Information) 검색
    while (tablePtr < tableEnd) {
        PSMBIOS_HEADER header = (PSMBIOS_HEADER)tablePtr;
        if (header->Type == 1) {
            PSMBIOS_SYSTEM_INFO sysInfo = (PSMBIOS_SYSTEM_INFO)header;

            // UUID를 16진수 문자열로 변환 (32자리)
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

        // 다음 구조체로 이동
        tablePtr += header->Length;
        while (tablePtr < tableEnd && (tablePtr[0] != 0 || tablePtr[1] != 0)) {
            tablePtr++;
        }
        tablePtr += 2;
    }

    // UUID가 없으면 NONE
    if (status != STATUS_SUCCESS) {
        RtlStringCbCopyA(Buffer, BufferSize, "NONE");
        status = STATUS_SUCCESS;
    }

    MmUnmapIoSpace(smbiosTable, tableSize);
    return status;
}

// CPU 정보 가져오기
// SMBIOS Type 4 (Processor)에서 ProcessorID 추출 (중복 제거)
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
    CHAR foundCPUs[8][17]; // 최대 8개 고유 CPU ID 저장
    ULONG uniqueCount = 0;

    if (!Buffer || BufferSize == 0) {
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(Buffer, BufferSize);
    RtlZeroMemory(foundCPUs, sizeof(foundCPUs));

    // SMBIOS 엔트리 포인트 검색
    physicalAddress.QuadPart = 0xF0000;
    mappedAddress = MmMapIoSpace(physicalAddress, 0x10000, MmNonCached);
    if (!mappedAddress) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // "_SM_" 시그니처 검색
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

    // SMBIOS 테이블 매핑
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

    // 모든 Type 4 (Processor) 순회
    while (tablePtr < tableEnd) {
        PSMBIOS_HEADER header = (PSMBIOS_HEADER)tablePtr;

        if (header->Type == 4) {
            PSMBIOS_PROCESSOR_INFO procInfo = (PSMBIOS_PROCESSOR_INFO)header;

            // ProcessorID를 16진수 문자열로 변환
            CHAR cpuId[17];
            RtlStringCchPrintfA(cpuId, sizeof(cpuId),
                "%02X%02X%02X%02X%02X%02X%02X%02X",
                procInfo->ProcessorID[7], procInfo->ProcessorID[6],
                procInfo->ProcessorID[5], procInfo->ProcessorID[4],
                procInfo->ProcessorID[3], procInfo->ProcessorID[2],
                procInfo->ProcessorID[1], procInfo->ProcessorID[0]);

            // 중복 체크 (멀티코어 CPU는 동일한 ID를 가질 수 있음)
            BOOLEAN isDuplicate = FALSE;
            for (ULONG i = 0; i < uniqueCount; i++) {
                if (strcmp(foundCPUs[i], cpuId) == 0) {
                    isDuplicate = TRUE;
                    break;
                }
            }

            // 중복이 아니면 저장
            if (!isDuplicate && uniqueCount < 8) {
                RtlStringCchCopyA(foundCPUs[uniqueCount], sizeof(foundCPUs[0]), cpuId);
                uniqueCount++;
            }
        }

        // 다음 구조체로 이동
        tablePtr += header->Length;
        while (tablePtr < tableEnd && (tablePtr[0] != 0 || tablePtr[1] != 0)) {
            tablePtr++;
        }
        tablePtr += 2;
    }

    // 중복 제거된 CPU ID들을 버퍼에 복사
    for (ULONG i = 0; i < uniqueCount; i++) {
        // 구분자 추가 (첫 번째가 아니면)
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

    // CPU를 하나도 못 찾았으면
    if (uniqueCount == 0) {
        RtlStringCbCopyA(Buffer, BufferSize, "NONE");
        status = STATUS_SUCCESS;
    }

    MmUnmapIoSpace(smbiosTable, tableSize);
    return status;
}

// SMBIOS 문자열 가져오기
// 구조체 뒤에 null-terminated 문자열들이 연속으로 저장됨
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

// 하드웨어 정보 응답 메시지 생성 (TLV 구조)
NTSTATUS BuildHardwareResponseMessage(ULONG RequestType, PUCHAR* Message, PULONG MessageSize)
{
    CHAR mainboardSerial[256] = { 0 };
    CHAR cpuInfo[256] = { 0 };
    CHAR memoryInfo[256] = { 0 };
    ULONG totalSize = sizeof(MESSAGE_HEADER);
    PUCHAR buffer = NULL;
    PUCHAR currentPtr = NULL;
    ULONG fieldCount = 0;

    // 요청 타입에 따라 정보 수집
    switch (RequestType) {
    case HW_REQUEST_ALL:
        GetMainboardSerial(mainboardSerial, sizeof(mainboardSerial));
        GetCPUInfo(cpuInfo, sizeof(cpuInfo));
        totalSize += sizeof(MESSAGE_FIELD) - 1 + strlen(mainboardSerial);
        totalSize += sizeof(MESSAGE_FIELD) - 1 + strlen(cpuInfo);
        fieldCount = 2;
        break;

    case HW_REQUEST_MAINBOARD:
        GetMainboardSerial(mainboardSerial, sizeof(mainboardSerial));
        totalSize += sizeof(MESSAGE_FIELD) - 1 + strlen(mainboardSerial);
        fieldCount = 1;
        break;

    case HW_REQUEST_CPU:
        GetCPUInfo(cpuInfo, sizeof(cpuInfo));
        totalSize += sizeof(MESSAGE_FIELD) - 1 + strlen(cpuInfo);
        fieldCount = 1;
        break;

    default:
        return STATUS_INVALID_PARAMETER;
    }

    // 응답 버퍼 동적 할당
    buffer = (PUCHAR)ExAllocatePoolWithTag(NonPagedPool, totalSize, 'wdHW');
    if (!buffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(buffer, totalSize);
    currentPtr = buffer;

    // 메시지 헤더 작성
    static LONG g_MessageIdCounter = 0;
    PMESSAGE_HEADER header = (PMESSAGE_HEADER)currentPtr;
    header->MessageType = MSG_TYPE_HARDWARE_RESPONSE;
    header->MessageId = (ULONG)InterlockedIncrement(&g_MessageIdCounter);
    header->FieldCount = (USHORT)fieldCount;
    header->Reserved = 0;
    currentPtr += sizeof(MESSAGE_HEADER);

    // 메인보드 필드 추가
    if (RequestType == HW_REQUEST_ALL || RequestType == HW_REQUEST_MAINBOARD) {
        PMESSAGE_FIELD field = (PMESSAGE_FIELD)currentPtr;
        field->FieldId = HW_REQUEST_MAINBOARD;
        field->DataSize = (ULONG)strlen(mainboardSerial);
        RtlCopyMemory(field->Data, mainboardSerial, field->DataSize);
        currentPtr += sizeof(MESSAGE_FIELD) - 1 + field->DataSize;
    }

    // CPU 필드 추가
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