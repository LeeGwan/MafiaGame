// 커널 드라이버 통신 인터페이스 정의
// DeviceIoControl을 통해 유저모드-커널모드 간 통신에 사용되는 상수 및 구조체
#pragma once
#include <Windows.h>

// 메시지 타입 정의
#define MSG_TYPE_HEARTBEAT          1  // 드라이버 생존 확인용 하트비트
#define MSG_TYPE_DISCONNECT         2  // 연결 종료 요청
#define MSG_TYPE_HARDWARE_REQUEST   3  // 하드웨어 정보 요청
#define MSG_TYPE_HARDWARE_RESPONSE  4  // 하드웨어 정보 응답

// 하드웨어 정보 요청 타입 (HWID 수집용)
#define HW_REQUEST_ALL          0  // 모든 하드웨어 정보 요청
#define HW_REQUEST_MAINBOARD    1  // 메인보드 UUID (SMBIOS 기반)
#define HW_REQUEST_CPU          2  // CPU 시리얼 번호
#define HW_REQUEST_STORAGE      4  // 스토리지 시리얼

// 보안 기능 요청 타입
#define SECURITY_REQUEST_OB_STATUS      5   // ObRegisterCallbacks 상태 조회
#define SECURITY_REQUEST_OB_REGISTER    6   // 프로세스 보호 콜백 등록
#define SECURITY_REQUEST_OB_UNREGISTER  7   // 프로세스 보호 콜백 해제
#define SECURITY_REQUEST_GET_ALERTS     8   // 공격 시도 로그 조회
#define SECURITY_REQUEST_ADD_PID        9   // 보호 대상 프로세스 추가
#define SECURITY_REQUEST_REMOVE_PID     10  // 보호 대상 프로세스 제거

// IOCTL 코드 정의 (DeviceIoControl의 dwIoControlCode 파라미터)
#define IOCTL_HARDWARE_GET_INFO    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)  // 하드웨어 정보 수집
#define IOCTL_HARDWARE_HEARTBEAT   CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)  // 하트비트 체크
#define IOCTL_SECURITY_CONTROL     CTL_CODE(FILE_DEVICE_UNKNOWN, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)  // 보안 기능 제어

#define MAX_ALERTS 100  // 최대 보안 경고 저장 개수

// 드라이버 통신 메시지 헤더
typedef struct _MESSAGE_HEADER {
    ULONG MessageType;   // 메시지 타입 (MSG_TYPE_*)
    ULONG MessageId;     // 메시지 고유 ID
    USHORT FieldCount;   // 포함된 필드 개수
    USHORT Reserved;     // 예약 (정렬용)
} MESSAGE_HEADER, * PMESSAGE_HEADER;

// 가변 길이 데이터 필드 (TLV 구조)
typedef struct _MESSAGE_FIELD {
    USHORT FieldId;    // 필드 타입 (HW_REQUEST_*)
    ULONG DataSize;    // 데이터 크기
    UCHAR Data[1];     // 실제 데이터 (가변 길이)
} MESSAGE_FIELD, * PMESSAGE_FIELD;

// 하드웨어 정보 요청 구조체
typedef struct _HARDWARE_REQUEST {
    ULONG RequestType;  // 요청 타입 (HW_REQUEST_*)
    ULONG Reserved;     // 예약
} HARDWARE_REQUEST, * PHARDWARE_REQUEST;

// 프로세스 보호 요청 구조체
typedef struct _PROTECTION_REQUEST {
    ULONG RequestType;  // 요청 타입 (SECURITY_REQUEST_*)
    HANDLE ProcessId;   // 대상 프로세스 ID (0이면 현재 프로세스)
    ULONG Reserved;     // 예약
} PROTECTION_REQUEST, * PPROTECTION_REQUEST;

// 프로세스 보호 상태 구조체
typedef struct _PROTECTION_STATUS {
    BOOLEAN ProtectionEnabled;      // 보호 기능 활성화 여부
    ULONG ProtectedCount;           // 보호 중인 프로세스 개수
    HANDLE ProtectedPIDs[50];       // 보호 중인 PID 배열
} PROTECTION_STATUS, * PPROTECTION_STATUS;

// 보안 경고 구조체 (공격 시도 로그)
typedef struct _SECURITY_ALERT {
    HANDLE AttackerPID;        // 공격자 프로세스 ID
    HANDLE TargetPID;          // 공격 대상 프로세스 ID
    CHAR AttackerName[16];     // 공격자 프로세스 이름
    LARGE_INTEGER Timestamp;   // 공격 시도 시간
    DWORD AttemptedAccess;     // 시도한 접근 권한 플래그
} SECURITY_ALERT, * PSECURITY_ALERT;