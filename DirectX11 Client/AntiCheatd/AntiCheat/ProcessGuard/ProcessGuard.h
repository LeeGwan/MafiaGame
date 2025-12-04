// 프로세스 보호 시스템 (ObRegisterCallbacks 기반)
// 보호 대상 프로세스에 대한 접근 차단 및 공격 시도 로깅
#pragma once
#include <ntddk.h>
#include "../Context/ProcessCodeHash/ProcessCodeHash.h"

#define MAX_ALERTS 100  // 최대 공격 시도 로그 개수

// 전역 변수 선언
extern PVOID g_ObCallbackHandle;                    // ObRegisterCallbacks 핸들
extern BOOLEAN g_ProtectionEnabled;                 // 보호 기능 활성화 여부
extern PROCESS_PROTECTED g_Process_Protected[10];   // 보호 대상 프로세스 배열 (최대 10개)
extern ULONG g_ProtectedCount;                      // 보호 중인 프로세스 개수
extern FAST_MUTEX g_ProtectionMutex;                // 보호 목록 동기화용 뮤텍스
extern KSPIN_LOCK g_AlertLock;                      // 공격 로그 동기화용 스핀락
extern SECURITY_ALERT g_AlertQueue[MAX_ALERTS];     // 공격 시도 로그 큐
extern ULONG g_AlertIndex;                          // 공격 로그 인덱스

// ObRegisterCallbacks 콜백 함수들
OB_PREOP_CALLBACK_STATUS PreOperationCallback(
    _In_ PVOID RegistrationContext,
    _In_ POB_PRE_OPERATION_INFORMATION OperationInformation
);

VOID PostOperationCallback(
    _In_ PVOID RegistrationContext,
    _In_ POB_POST_OPERATION_INFORMATION OperationInformation
);

OB_PREOP_CALLBACK_STATUS ThreadPreOperationCallback(
    _In_ PVOID RegistrationContext,
    _In_ POB_PRE_OPERATION_INFORMATION OperationInformation
);

// 언더큐먼트 함수 선언
NTKERNELAPI
PEPROCESS
IoThreadToProcess(
    _In_ PETHREAD Thread
);

// 프로세스 보호 관련 함수
NTSTATUS EnableProcessProtection(VOID);                        // ObRegisterCallbacks 등록
BOOLEAN IsProcessProtected(HANDLE ProcessId);                  // 보호 대상 여부 확인
VOID AddSecurityAlert(HANDLE AttackerPID, HANDLE TargetPID,   // 공격 로그 추가
    PCHAR AttackerName, ACCESS_MASK Access);
NTSTATUS DisableProcessProtection(VOID);                       // ObRegisterCallbacks 해제
NTSTATUS AddProtectedProcess(HANDLE ProcessId);                // 보호 대상 프로세스 추가
NTSTATUS GetAlert_Queue(PVOID outputBuffer, ULONG outputLength,  // 공격 로그 조회
    PULONG bytesTransferred);