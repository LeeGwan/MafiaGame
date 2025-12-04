// CRC32 기반 프로세스 코드 무결성 검증 구현부
// PE 파일의 .text 및 .rdata 섹션을 256바이트 단위로 분할하여 CRC32 해시 생성
// 10초마다 메모리를 재검증하여 코드 변조 탐지
#include "crc.h"
#include <Ntstrsafe.h>

// 전역 변수 정의
ULONG g_SelfFunctionSize = 0x1000;
ULONG CRC32Table[256] = { 0 };
BOOLEAN CRC32Initialized = FALSE;
PROCESS_CODE_HASH g_process_code_hash[10];
ULONG g_process_code_hashCount = 0;
BOOLEAN g_IntegrityCheckRunning = FALSE;
PKTHREAD g_IntegrityCheckThread = NULL;

// CRC32 룩업 테이블 초기화 (0xEDB88320 다항식 사용)
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

// CRC32 해시 계산
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

// 프로세스 메모리 읽기 (MmCopyVirtualMemory 사용)
NTSTATUS ReadProcessMemory(PEPROCESS Process, PVOID Address, PVOID Buffer, SIZE_T Size)
{
    NTSTATUS status;
    SIZE_T bytesRead = 0;

    if (!Process || !Address || !Buffer || Size == 0) {
        return STATUS_INVALID_PARAMETER;
    }

    // 다른 프로세스 메모리 읽기
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

// 프로세스의 메인 모듈 베이스 주소 가져오기 (PEB->ImageBaseAddress)
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

        // 프로세스 컨텍스트로 전환
        KeStackAttachProcess((PKPROCESS)Process, &apcState);

        if (MmIsAddressValid(peb)) {
            // PEB 오프셋 0x10에 ImageBaseAddress 위치
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

// PE 파일의 .text 및 .rdata 섹션 찾기
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

        // DOS 헤더 검증
        PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)ImageBase;
        if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
            DbgPrint("Invalid DOS signature\n");
            goto Exit;
        }

        // NT 헤더 검증
        PIMAGE_NT_HEADERS64 ntHeaders = (PIMAGE_NT_HEADERS64)((PUCHAR)ImageBase + dosHeader->e_lfanew);
        if (!MmIsAddressValid(ntHeaders)) {
            goto Exit;
        }

        if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
            DbgPrint("Invalid PE signature\n");
            goto Exit;
        }

        // 섹션 헤더 배열 시작 주소
        PIMAGE_SECTION_HEADER sectionHeader = (PIMAGE_SECTION_HEADER)(
            (PUCHAR)ntHeaders + sizeof(IMAGE_NT_HEADERS64)
            );

        USHORT numberOfSections = ntHeaders->FileHeader.NumberOfSections;

        // 모든 섹션 순회하여 .text 및 .rdata 찾기
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

// 프로세스 코드 해시 생성 (256바이트 단위)
NTSTATUS AddCRC(HANDLE ProcessId)
{
    NTSTATUS status;
    PEPROCESS process = NULL;
    PUCHAR codeBuffer = NULL;
    PVOID imageBase = NULL;
    CODE_SECTION_INFO sections[2];
    ULONG totalChunkCount = 0;

    // 프로세스 객체 가져오기
    status = PsLookupProcessByProcessId(ProcessId, &process);
    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed to lookup process %d: 0x%08X\n",
            HandleToULong(ProcessId), status);
        return status;
    }

    // 메인 모듈 베이스 주소 가져오기
    imageBase = GetProcessMainModuleBase(process);
    if (!imageBase) {
        DbgPrint("Failed to get base address for PID %d\n",
            HandleToULong(ProcessId));
        ObDereferenceObject(process);
        return STATUS_UNSUCCESSFUL;
    }

    // .text 및 .rdata 섹션 찾기
    ULONG sectionCount = FindCodeSections(process, imageBase, sections);
    if (sectionCount == 0) {
        DbgPrint("Failed to find code sections for PID %d\n",
            HandleToULong(ProcessId));
        ObDereferenceObject(process);
        return STATUS_UNSUCCESSFUL;
    }

    // 전체 청크 개수 계산
    for (ULONG s = 0; s < sectionCount; s++) {
        totalChunkCount += (sections[s].Size + CODE_CHUNK_SIZE - 1) / CODE_CHUNK_SIZE;
    }

    // 해시 배열 동적 할당
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

    // 코드 읽기용 버퍼 할당
    codeBuffer = (PUCHAR)ExAllocatePoolWithTag(NonPagedPool, CODE_CHUNK_SIZE, 'hCIC');
    if (!codeBuffer) {
        ExFreePool(hashValues);
        ObDereferenceObject(process);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // 각 섹션을 256바이트 단위로 분할하여 CRC32 계산
    ULONG hashIndex = 0;
    for (ULONG s = 0; s < sectionCount; s++) {
        ULONG sectionSize = sections[s].Size;
        PVOID sectionAddress = sections[s].Address;
        ULONG chunkCount = (sectionSize + CODE_CHUNK_SIZE - 1) / CODE_CHUNK_SIZE;

        for (ULONG i = 0; i < chunkCount; i++) {
            ULONG offset = i * CODE_CHUNK_SIZE;
            ULONG currentChunkSize = CODE_CHUNK_SIZE;

            // 마지막 청크는 크기가 작을 수 있음
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

            // CRC32 해시 계산
            hashValues[hashIndex] = CalculateCRC32(codeBuffer, currentChunkSize);
            hashIndex++;
        }
    }

    // g_process_code_hash 배열에 저장
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

// 프로세스 코드 해시 검증 (메모리 변조 탐지)
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

    // 각 청크를 읽어서 CRC32 재계산 후 비교
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

            // 현재 CRC32 계산
            ULONG currentHash = CalculateCRC32(codeBuffer, currentChunkSize);

            // 원본 해시와 비교
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

// 무결성 검증 시스템 초기화
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

    // 무결성 검증 스레드 생성
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

    // 스레드 객체 참조
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

// 무결성 검증 시스템 종료
VOID StopIntegrityCheck(VOID)
{
    if (!g_IntegrityCheckRunning) {
        return;
    }

    g_IntegrityCheckRunning = FALSE;

    // 스레드 종료 대기
    if (g_IntegrityCheckThread) {
        KeWaitForSingleObject(g_IntegrityCheckThread, Executive, KernelMode, FALSE, NULL);
        ObDereferenceObject(g_IntegrityCheckThread);
        g_IntegrityCheckThread = NULL;
    }

    // 해시 배열 해제
    for (ULONG i = 0; i < 10; i++) {
        if (g_process_code_hash[i].HashValues != NULL) {
            ExFreePool(g_process_code_hash[i].HashValues);
            g_process_code_hash[i].HashValues = NULL;
        }
    }

    RtlZeroMemory(g_process_code_hash, sizeof(g_process_code_hash));
}

// 무결성 검증 스레드 루틴 (10초마다 검증)
VOID IntegrityCheckThreadRoutine(PVOID Context)
{
    UNREFERENCED_PARAMETER(Context);

    while (g_IntegrityCheckRunning) {
        // 등록된 모든 프로세스 검증
        if (g_process_code_hashCount > 0) {
            for (ULONG i = 0; i < 10; i++) {
                if (g_process_code_hash[i].IsValid && g_process_code_hash[i].ProcessId != NULL) {
                    BOOLEAN result = ComputeProcessCodeHash(&g_process_code_hash[i]);

                    if (!result) {
                        // 메모리 변조 탐지 시 추가 조치 가능
                    }
                }
            }
        }

        // 10초 대기
        LARGE_INTEGER interval;
        interval.QuadPart = -100000000LL;  // 10초 (100ns 단위)
        KeDelayExecutionThread(KernelMode, FALSE, &interval);
    }

    DbgPrint("Integrity check thread stopped\n");
    PsTerminateSystemThread(STATUS_SUCCESS);
}