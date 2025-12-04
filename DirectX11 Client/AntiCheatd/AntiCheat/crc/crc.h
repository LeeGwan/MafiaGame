// CRC32 기반 프로세스 코드 무결성 검증
// .text 및 .rdata 섹션을 256바이트 단위로 분할하여 CRC32 해시 생성
#pragma once
#include <ntddk.h>
#include "../Context/ProcessCodeHash/ProcessCodeHash.h"

// 전역 변수 선언
extern ULONG g_SelfFunctionSize;                 // 함수 크기 (미사용)
extern PROCESS_CODE_HASH g_process_code_hash[10];  // 프로세스별 코드 해시 배열
extern ULONG g_process_code_hashCount;           // 등록된 프로세스 개수
extern ULONG CRC32Table[256];                    // CRC32 룩업 테이블
extern BOOLEAN CRC32Initialized;                 // CRC32 테이블 초기화 여부
extern BOOLEAN g_IntegrityCheckRunning;          // 무결성 검증 스레드 실행 여부
extern PKTHREAD g_IntegrityCheckThread;          // 무결성 검증 스레드 객체

// 언더큐먼트 함수 선언
NTKERNELAPI
VOID
KeStackAttachProcess(
    _In_ PKPROCESS Process,
    _Out_ PKAPC_STATE ApcState
);

NTKERNELAPI
VOID
KeUnstackDetachProcess(
    _In_ PKAPC_STATE ApcState
);

NTKERNELAPI
PPEB
PsGetProcessPeb(
    _In_ PEPROCESS Process
);

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

// 무결성 검증 시스템 초기화 및 종료
NTSTATUS InitializeIntegrityCheck(VOID);
VOID StopIntegrityCheck(VOID);

// CRC32 관련 함수
VOID InitializeCRC32Table(VOID);
ULONG CalculateCRC32(PUCHAR Data, ULONG Length);

// 프로세스 메모리 읽기
NTSTATUS ReadProcessMemory(PEPROCESS Process, PVOID Address, PVOID Buffer, SIZE_T Size);

// PE 파싱 및 코드 섹션 추출
PVOID GetProcessMainModuleBase(PEPROCESS Process);
ULONG FindCodeSections(PEPROCESS Process, PVOID ImageBase, CODE_SECTION_INFO Sections[2]);

// 프로세스 코드 해시 생성 및 검증
NTSTATUS AddCRC(HANDLE ProcessId);
BOOLEAN ComputeProcessCodeHash(PPROCESS_CODE_HASH hashInfo);

// 무결성 검증 스레드 루틴 (10초마다 검증)
VOID IntegrityCheckThreadRoutine(PVOID Context);