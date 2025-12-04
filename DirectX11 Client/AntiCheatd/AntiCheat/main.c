// 커널 드라이버 엔트리 포인트 및 IOCTL 핸들러
// Flect 안티치트 드라이버 메인 파일
#include "GetHardware/GetHardware.h"
#include "ProcessGuard/ProcessGuard.h"
#include "Context/HwSecurityProtocol/HwSecurityProtocol.h"
#include "Context/HardWareContext/HardWareContext.h"
#include"crc/crc.h"

// 전역 변수
BOOLEAN g_DriverActive;           // 드라이버 활성화 상태
UNICODE_STRING g_DeviceName;      // 디바이스 이름 (\Device\Flect)
UNICODE_STRING g_SymbolicLinkName;  // 심볼릭 링크 이름 (\DosDevices\Flect)
PDEVICE_OBJECT g_DeviceObject;    // 디바이스 객체

// 함수 선언
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
VOID DriverUnload(PDRIVER_OBJECT DriverObject);
NTSTATUS DeviceCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS DeviceClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS DeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

// 드라이버 엔트리 포인트
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(RegistryPath);

    // 디바이스 이름 및 심볼릭 링크 초기화
    RtlInitUnicodeString(&g_DeviceName, L"\\Device\\Flect");
    RtlInitUnicodeString(&g_SymbolicLinkName, L"\\DosDevices\\Flect");

    // 디바이스 객체 생성
    status = IoCreateDevice(
        DriverObject,
        0,                          // 디바이스 확장 크기 (사용 안 함)
        &g_DeviceName,
        FILE_DEVICE_UNKNOWN,        // 디바이스 타입
        FILE_DEVICE_SECURE_OPEN,    // 보안 열기
        FALSE,                      // 독점 액세스 안 함
        &g_DeviceObject
    );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    // 심볼릭 링크 생성 (유저모드에서 \\.\Flect로 접근 가능)
    status = IoCreateSymbolicLink(&g_SymbolicLinkName, &g_DeviceName);
    if (!NT_SUCCESS(status)) {
        DbgPrint("[DriverEntry] Failed to create symbolic link: 0x%08X\n", status);
        IoDeleteDevice(g_DeviceObject);
        return status;
    }

    // IRP 핸들러 등록
    DriverObject->MajorFunction[IRP_MJ_CREATE] = DeviceCreate;         // CreateFile
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DeviceClose;           // CloseHandle
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceControl;  // DeviceIoControl
    DriverObject->DriverUnload = DriverUnload;

    g_DriverActive = TRUE;
    DbgPrint("Flect Driver was successfully loaded to check the program\n");

    return STATUS_SUCCESS;
}

// 드라이버 언로드
VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);

    // 프로세스 보호 해제
    if (g_ProtectionEnabled) {
        DisableProcessProtection();
    }

    g_DriverActive = FALSE;

    // 심볼릭 링크 삭제
    IoDeleteSymbolicLink(&g_SymbolicLinkName);

    // 디바이스 객체 삭제
    if (g_DeviceObject) {
        IoDeleteDevice(g_DeviceObject);
    }

    DbgPrint("HardwareInfo Driver unloaded\n");
}

// 디바이스 열기 (CreateFile 호출 시)
NTSTATUS DeviceCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DbgPrint("HardwareInfo Device opened\n");

    return STATUS_SUCCESS;
}

// 디바이스 닫기 (CloseHandle 호출 시)
NTSTATUS DeviceClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DbgPrint("HardwareInfo Device closed\n");

    return STATUS_SUCCESS;
}

// IOCTL 핸들러 (DeviceIoControl 호출 시)
NTSTATUS DeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpStack;
    ULONG ioControlCode;
    PVOID inputBuffer;
    PVOID outputBuffer;
    ULONG inputLength;
    ULONG outputLength;
    ULONG bytesTransferred = 0;

    UNREFERENCED_PARAMETER(DeviceObject);

    // 드라이버 활성화 상태 확인
    if (!g_DriverActive) {
        Irp->IoStatus.Status = STATUS_DEVICE_NOT_READY;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_DEVICE_NOT_READY;
    }

    // IRP 스택 위치 및 버퍼 정보 가져오기
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;
    inputBuffer = Irp->AssociatedIrp.SystemBuffer;   // METHOD_BUFFERED 사용
    outputBuffer = Irp->AssociatedIrp.SystemBuffer;
    inputLength = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outputLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    // IOCTL 코드별 분기 처리
    switch (ioControlCode)
    {
        // 하드웨어 정보 수집 (CPU ID, 메인보드 UUID)
    case IOCTL_HARDWARE_GET_INFO:
    {
        if (inputLength < sizeof(HARDWARE_REQUEST)) {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        PHARDWARE_REQUEST request = (PHARDWARE_REQUEST)inputBuffer;
        PUCHAR responseMessage = NULL;
        ULONG messageSize = 0;

        // SMBIOS 파싱하여 하드웨어 정보 수집
        status = BuildHardwareResponseMessage(request->RequestType, &responseMessage, &messageSize);

        if (NT_SUCCESS(status)) {
            if (outputLength >= messageSize) {
                // 응답 메시지 복사
                RtlCopyMemory(outputBuffer, responseMessage, messageSize);
                bytesTransferred = messageSize;
            }
            else {
                status = STATUS_BUFFER_TOO_SMALL;
            }

            // 동적 할당된 메시지 해제
            if (responseMessage) {
                ExFreePool(responseMessage);
            }
        }
    }
    break;

    // 하트비트 체크 (드라이버 무결성 검증)
    case IOCTL_HARDWARE_HEARTBEAT:
    {
        ULONG heartbeatResponse = 0x12345678;  // 테스트용 고정 값
        if (outputLength >= sizeof(ULONG)) {
            *(PULONG)outputBuffer = heartbeatResponse;
            bytesTransferred = sizeof(ULONG);
        }
        else {
            status = STATUS_BUFFER_TOO_SMALL;
        }
    }
    break;

    // 보안 기능 제어 (ObRegisterCallbacks, PID 추가, 공격 로그 조회)
    case IOCTL_SECURITY_CONTROL:
    {
        if (inputLength < sizeof(PROTECTION_REQUEST)) {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        PPROTECTION_REQUEST protRequest = (PPROTECTION_REQUEST)inputBuffer;

        switch (protRequest->RequestType) {
            // ObRegisterCallbacks 등록
        case SECURITY_REQUEST_OB_REGISTER:
            status = EnableProcessProtection();
            break;

            // 보호 대상 프로세스 추가
        case SECURITY_REQUEST_ADD_PID:
            status = AddProtectedProcess(protRequest->ProcessId);
            break;

            // 공격 시도 로그 조회
        case SECURITY_REQUEST_GET_ALERTS:
            status = GetAlert_Queue(outputBuffer, outputLength, &bytesTransferred);
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
            break;
        }
    }
    break;

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    // IRP 완료
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = bytesTransferred;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}