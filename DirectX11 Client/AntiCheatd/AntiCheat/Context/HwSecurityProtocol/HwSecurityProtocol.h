// 하드웨어 및 보안 프로토콜 정의
// 유저모드-커널모드 간 통신 프로토콜
#pragma once

// 메시지 타입 정의
#define MSG_TYPE_HEARTBEAT          1  // 드라이버 생존 확인
#define MSG_TYPE_DISCONNECT         2  // 연결 종료
#define MSG_TYPE_HARDWARE_REQUEST   3  // 하드웨어 정보 요청
#define MSG_TYPE_HARDWARE_RESPONSE  4  // 하드웨어 정보 응답

// 하드웨어 정보 요청 타입
#define HW_REQUEST_ALL          0  // 모든 하드웨어 정보
#define HW_REQUEST_MAINBOARD    1  // 메인보드 UUID (SMBIOS 기반)
#define HW_REQUEST_CPU          2  // CPU 시리얼 번호

#define HW_REQUEST_STORAGE      4  // 스토리지 시리얼 (미사용)

// 보안 요청 타입
#define SECURITY_REQUEST_OB_STATUS      5   // ObRegisterCallbacks 상태 조회
#define SECURITY_REQUEST_OB_REGISTER    6   // 프로세스 보호 활성화
#define SECURITY_REQUEST_OB_UNREGISTER  7   // 프로세스 보호 해제
#define SECURITY_REQUEST_GET_ALERTS     8   // 공격 시도 로그 조회
#define SECURITY_REQUEST_ADD_PID        9   // 보호 대상 프로세스 추가
#define SECURITY_REQUEST_REMOVE_PID     10  // 보호 대상 프로세스 제거

// IOCTL 코드 (DeviceIoControl에서 사용)
#define IOCTL_HARDWARE_GET_INFO    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)  // 하드웨어 정보 수집
#define IOCTL_HARDWARE_HEARTBEAT   CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)  // 하트비트 체크
#define IOCTL_SECURITY_CONTROL     CTL_CODE(FILE_DEVICE_UNKNOWN, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)  // 보안 기능 제어