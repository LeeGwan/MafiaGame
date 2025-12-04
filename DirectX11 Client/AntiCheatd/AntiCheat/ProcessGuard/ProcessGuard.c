// 프로세스 보호 시스템 구현부
// ObRegisterCallbacks를 통한 프로세스/스레드 핸들 접근 차단
#include "ProcessGuard.h"
#include"../Context/SecurityAccessFlags/SecurityAccessFlags.h"
#include <Ntstrsafe.h>
#include "../crc/crc.h"

// 전역 변수 정의
PVOID g_ObCallbackHandle = NULL;              // ObRegisterCallbacks 핸들
BOOLEAN g_ProtectionEnabled = FALSE;          // 보호 기능 활성화 여부
PROCESS_PROTECTED g_Process_Protected[10];    // 보호 대상 프로세스 배열
ULONG g_ProtectedCount = 0;                   // 보호 중인 프로세스 개수
FAST_MUTEX g_ProtectionMutex;                 // 보호 목록 동기화용 뮤텍스
KSPIN_LOCK g_AlertLock;                       // 공격 로그 동기화용 스핀락
SECURITY_ALERT g_AlertQueue[MAX_ALERTS];      // 공격 시도 로그 큐
ULONG g_AlertIndex = 0;                       // 공격 로그 인덱스

// ObRegisterCallbacks 등록 및 프로세스 보호 활성화
NTSTATUS EnableProcessProtection(VOID)
{
    NTSTATUS status;
    OB_CALLBACK_REGISTRATION callbackReg;
    OB_OPERATION_REGISTRATION operationReg[2];

    // 이미 활성화되어 있으면 성공 반환
    if (g_ProtectionEnabled) {
        DbgPrint("Process protection already enabled\n");
        return STATUS_SUCCESS;
    }

    // 동기화 객체 초기화
    ExInitializeFastMutex(&g_ProtectionMutex);
    KeInitializeSpinLock(&g_AlertLock);

    // 콜백 등록 구조체 초기화
    RtlZeroMemory(&callbackReg, sizeof(OB_CALLBACK_REGISTRATION));
    RtlZeroMemory(&operationReg, sizeof(OB_OPERATION_REGISTRATION) * 2);

    callbackReg.Version = ObGetFilterVersion();
    callbackReg.OperationRegistrationCount = 2;  // 프로세스 + 스레드
    callbackReg.RegistrationContext = NULL;
    RtlInitUnicodeString(&callbackReg.Altitude, L"300000");  // 콜백 우선순위

    // 프로세스 핸들 콜백 등록
    operationReg[0].ObjectType = PsProcessType;
    operationReg[0].Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
    operationReg[0].PreOperation = PreOperationCallback;    // 핸들 생성/복제 전 호출
    operationReg[0].PostOperation = PostOperationCallback;  // 핸들 생성/복제 후 호출

    // 스레드 핸들 콜백 등록
    operationReg[1].ObjectType = PsThreadType;
    operationReg[1].Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
    operationReg[1].PreOperation = ThreadPreOperationCallback;
    operationReg[1].PostOperation = NULL;

    callbackReg.OperationRegistration = operationReg;

    // ObRegisterCallbacks 호출
    status = ObRegisterCallbacks(&callbackReg, &g_ObCallbackHandle);

    if (NT_SUCCESS(status)) {
        g_ProtectionEnabled = TRUE;

        // CRC32 기반 코드 무결성 검증 초기화
        status = InitializeIntegrityCheck();
        if (NT_SUCCESS(status))
        {
            DbgPrint("Succesed to CRC: 0x%08X\n", status);
        }
        else
        {
            DbgPrint("Failed to CRC: 0x%08X\n", status);
        }
    }
    else {
        DbgPrint("Failed to enable protection: 0x%08X\n", status);
    }

    return status;
}

// 프로세스 핸들 생성/복제 전 콜백 (접근 권한 차단)
OB_PREOP_CALLBACK_STATUS PreOperationCallback(
    _In_ PVOID RegistrationContext,
    _In_ POB_PRE_OPERATION_INFORMATION OperationInformation
)
{
    UNREFERENCED_PARAMETER(RegistrationContext);

    PEPROCESS TargetProcess = (PEPROCESS)OperationInformation->Object;
    HANDLE TargetPID = PsGetProcessId(TargetProcess);
    HANDLE CurrentPID = PsGetCurrentProcessId();

    // 자기 자신은 허용
    if (TargetPID == CurrentPID) {
        return OB_PREOP_SUCCESS;
    }

    // 보호 대상 프로세스인지 확인
    if (IsProcessProtected(TargetPID)) {
        PEPROCESS CurrentProcess = PsGetCurrentProcess();

        // 시스템 프로세스 (PID 4)는 허용
        if (PsGetProcessId(CurrentProcess) == (HANDLE)4) {
            return OB_PREOP_SUCCESS;
        }

        // 핸들 생성 시 위험한 권한 차단
        if (OperationInformation->Operation == OB_OPERATION_HANDLE_CREATE) {
            if (OperationInformation->Parameters->CreateHandleInformation.OriginalDesiredAccess & PROTECT_FULL_ACCESS) {
                // 권한을 0으로 설정하여 차단
                OperationInformation->Parameters->CreateHandleInformation.DesiredAccess = 0;

                DbgPrint("[BLOCKED] Process handle creation denied for PID %d from PID %d\n",
                    HandleToULong(TargetPID), HandleToULong(CurrentPID));
            }
        }

        // 핸들 복제 시 위험한 권한 차단
        if (OperationInformation->Operation == OB_OPERATION_HANDLE_DUPLICATE) {
            if (OperationInformation->Parameters->DuplicateHandleInformation.OriginalDesiredAccess & PROTECT_FULL_ACCESS) {
                OperationInformation->Parameters->DuplicateHandleInformation.DesiredAccess = 0;

                DbgPrint("[BLOCKED] Process handle duplication denied for PID %d from PID %d\n",
                    HandleToULong(TargetPID), HandleToULong(CurrentPID));
            }
        }
    }

    return OB_PREOP_SUCCESS;
}

// 프로세스 핸들 생성/복제 후 콜백 (공격 시도 로깅)
VOID PostOperationCallback(
    _In_ PVOID RegistrationContext,
    _In_ POB_POST_OPERATION_INFORMATION OperationInformation
)
{
    UNREFERENCED_PARAMETER(RegistrationContext);

    PEPROCESS TargetProcess = (PEPROCESS)OperationInformation->Object;
    HANDLE TargetPID = PsGetProcessId(TargetProcess);

    // 보호 대상 프로세스인지 확인
    if (IsProcessProtected(TargetPID)) {
        PEPROCESS CurrentProcess = PsGetCurrentProcess();
        HANDLE CurrentPID = PsGetCurrentProcessId();

        // 자기 자신은 제외
        if (CurrentPID == TargetPID) {
            return;
        }

        // 시스템 프로세스는 제외
        if (CurrentPID == (HANDLE)4) {
            return;
        }

        // 공격자 프로세스 정보 수집
        PCHAR processName = (PCHAR)PsGetProcessImageFileName(CurrentProcess);
        ACCESS_MASK grantedAccess = 0;

        if (OperationInformation->Operation == OB_OPERATION_HANDLE_CREATE) {
            grantedAccess = OperationInformation->Parameters->CreateHandleInformation.GrantedAccess;
        }
        else if (OperationInformation->Operation == OB_OPERATION_HANDLE_DUPLICATE) {
            grantedAccess = OperationInformation->Parameters->DuplicateHandleInformation.GrantedAccess;
        }

        DbgPrint("[ALERT] Process '%s' (PID %d) accessed protected PID %d with access 0x%08X\n",
            processName, HandleToULong(CurrentPID), HandleToULong(TargetPID), grantedAccess);

        // 공격 시도 로그 추가
        AddSecurityAlert(CurrentPID, TargetPID, processName, grantedAccess);
    }
}

// 스레드 핸들 생성/복제 전 콜백 (접근 권한 차단)
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

    // 현재 테스트용으로 조기 반환 (스레드 보호 비활성화)
    return OB_PREOP_SUCCESS;

    // 자기 자신은 허용
    if (TargetPID == CurrentPID) {
        return OB_PREOP_SUCCESS;
    }

    // 보호 대상 프로세스인지 확인
    if (IsProcessProtected(TargetPID)) {

        // 시스템 프로세스는 허용
        if (CurrentPID == (HANDLE)4) {
            return OB_PREOP_SUCCESS;
        }

        // 스레드 핸들 생성 시 위험한 권한 차단
        if (OperationInformation->Operation == OB_OPERATION_HANDLE_CREATE) {

            if (OperationInformation->Parameters->CreateHandleInformation.OriginalDesiredAccess & PROTECT_THREAD_ACCESS) {

                OperationInformation->Parameters->CreateHandleInformation.DesiredAccess = 0;

                DbgPrint("[BLOCKED] Thread handle creation denied for protected PID %d from PID %d\n",
                    HandleToULong(TargetPID), HandleToULong(CurrentPID));
            }
        }

        // 스레드 핸들 복제 시 위험한 권한 차단
        if (OperationInformation->Operation == OB_OPERATION_HANDLE_DUPLICATE) {

            if (OperationInformation->Parameters->DuplicateHandleInformation.OriginalDesiredAccess & PROTECT_THREAD_ACCESS) {

                OperationInformation->Parameters->DuplicateHandleInformation.DesiredAccess = 0;

                DbgPrint("[BLOCKED] Thread handle duplication denied for protected PID %d from PID %d\n",
                    HandleToULong(TargetPID), HandleToULong(CurrentPID));
            }
        }
    }

    return OB_PREOP_SUCCESS;
}

// 프로세스가 보호 대상인지 확인
BOOLEAN IsProcessProtected(HANDLE ProcessId)
{
    BOOLEAN protected = FALSE;

    ExAcquireFastMutex(&g_ProtectionMutex);

    // 보호 목록 순회
    for (ULONG i = 0; i < g_ProtectedCount; i++) {
        if (g_Process_Protected[i].ProcessId == ProcessId) {
            protected = TRUE;
            break;
        }
    }

    ExReleaseFastMutex(&g_ProtectionMutex);
    return protected;
}

// 공격 시도 로그 추가 (순환 큐 방식)
VOID AddSecurityAlert(HANDLE AttackerPID, HANDLE TargetPID, PCHAR AttackerName, ACCESS_MASK Access)
{
    KIRQL oldIrql;
    KeAcquireSpinLock(&g_AlertLock, &oldIrql);

    // 순환 큐 방식으로 로그 저장
    PSECURITY_ALERT alert = &g_AlertQueue[g_AlertIndex % MAX_ALERTS];
    alert->AttackerPID = AttackerPID;
    alert->TargetPID = TargetPID;
    RtlStringCbCopyA(alert->AttackerName, sizeof(alert->AttackerName), AttackerName);
    KeQuerySystemTime(&alert->Timestamp);
    alert->AttemptedAccess = Access;

    g_AlertIndex++;

    KeReleaseSpinLock(&g_AlertLock, oldIrql);
}

// 프로세스 보호 해제
NTSTATUS DisableProcessProtection(VOID)
{
    // 보호 기능이 활성화되어 있지 않으면 실패
    if (!g_ProtectionEnabled || !g_ObCallbackHandle) {
        return STATUS_NOT_FOUND;
    }

    // CRC32 무결성 검증 중지
    StopIntegrityCheck();

    // ObRegisterCallbacks 콜백 해제
    ObUnRegisterCallbacks(g_ObCallbackHandle);
    g_ObCallbackHandle = NULL;
    g_ProtectionEnabled = FALSE;

    // 보호 목록 초기화
    ExAcquireFastMutex(&g_ProtectionMutex);
    RtlZeroMemory(g_Process_Protected, sizeof(g_Process_Protected));
    g_ProtectedCount = 0;
    ExReleaseFastMutex(&g_ProtectionMutex);

    DbgPrint("Process and Thread protection disabled\n");
    return STATUS_SUCCESS;
}

// 보호 대상 프로세스 추가
NTSTATUS AddProtectedProcess(HANDLE ProcessId)
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS Process = NULL;

    ExAcquireFastMutex(&g_ProtectionMutex);

    // 중복 체크
    for (ULONG i = 0; i < g_ProtectedCount; i++) {
        if (g_Process_Protected[i].ProcessId == ProcessId) {
            status = STATUS_ALREADY_REGISTERED;
            goto Exit;
        }
    }

    // 최대 개수 확인 (10개로 제한)
    if (g_ProtectedCount >= 10) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    // 프로세스 유효성 확인
    status = PsLookupProcessByProcessId(ProcessId, &Process);
    if (!NT_SUCCESS(status)) {
        goto Exit;
    }

    // 프로세스를 보호 목록에 추가
    g_Process_Protected[g_ProtectedCount].ProcessId = ProcessId;
    g_Process_Protected[g_ProtectedCount].IsValid = TRUE;
    g_ProtectedCount++;

    // CRC32 기반 코드 해시 생성 (.text, .rdata 섹션)
    status = AddCRC(ProcessId);
    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed AddCRC\n");
    }

    ObDereferenceObject(Process);
    ExReleaseFastMutex(&g_ProtectionMutex);

    return STATUS_SUCCESS;

Exit:
    ExReleaseFastMutex(&g_ProtectionMutex);
    return status;
}

// 공격 시도 로그 조회 (유저모드로 전송)
NTSTATUS GetAlert_Queue(PVOID outputBuffer, ULONG outputLength, PULONG bytesTransferred)
{
    NTSTATUS status;

    // 출력 버퍼 크기 확인
    if (outputLength >= sizeof(SECURITY_ALERT) * MAX_ALERTS) {
        KIRQL oldIrql;
        KeAcquireSpinLock(&g_AlertLock, &oldIrql);

        // 로그가 없으면 STATUS_NO_MORE_ENTRIES 반환
        if (g_AlertIndex == 0) {
            status = STATUS_NO_MORE_ENTRIES;
            *bytesTransferred = 0;
        }
        else {
            // 전체 로그 큐를 유저모드로 복사
            RtlCopyMemory(outputBuffer, g_AlertQueue, sizeof(g_AlertQueue));
            *bytesTransferred = sizeof(g_AlertQueue);

            // 로그 큐 초기화
            RtlZeroMemory(g_AlertQueue, sizeof(g_AlertQueue));
            g_AlertIndex = 0;
            status = STATUS_SUCCESS;
        }

        KeReleaseSpinLock(&g_AlertLock, oldIrql);
    }
    else {
        status = STATUS_BUFFER_TOO_SMALL;
    }

    return status;
}