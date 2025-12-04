// 하드웨어 정보 관련 구조체 정의
// SMBIOS 파싱 및 메시지 프로토콜
#pragma once

#include <ntddk.h>

// 메시지 헤더 
typedef struct _MESSAGE_HEADER {
    ULONG MessageType;   // 메시지 타입 (MSG_TYPE_*)
    ULONG MessageId;     // 메시지 고유 ID
    USHORT FieldCount;   // 포함된 필드 개수
    USHORT Reserved;     // 예약
} MESSAGE_HEADER, * PMESSAGE_HEADER;

// 가변 길이 필드
typedef struct _MESSAGE_FIELD {
    USHORT FieldId;    // 필드 타입 (HW_REQUEST_*)
    ULONG DataSize;    // 데이터 크기
    UCHAR Data[1];     // 실제 데이터 (가변 길이)
} MESSAGE_FIELD, * PMESSAGE_FIELD;

// SMBIOS 엔트리 포인트 (0xF0000 ~ 0xFFFFF 영역에 위치)
typedef struct _SMBIOS_ENTRY_POINT {
    UCHAR Anchor[4];              // "_SM_" 시그니처
    UCHAR Checksum;               // 체크섬
    UCHAR Length;                 // 구조체 길이
    UCHAR MajorVersion;           // SMBIOS 메이저 버전
    UCHAR MinorVersion;           // SMBIOS 마이너 버전
    USHORT MaxStructureSize;      // 최대 구조체 크기
    UCHAR EntryPointRevision;     // 엔트리 포인트 리비전
    UCHAR FormattedArea[5];       // 포맷된 영역
    UCHAR IntermediateAnchor[5];  // "_DMI_" 시그니처
    UCHAR IntermediateChecksum;   // 중간 체크섬
    USHORT TableLength;           // 테이블 길이
    ULONG TableAddress;           // 테이블 물리 주소
    USHORT NumberOfStructures;    // 구조체 개수
    UCHAR BCDRevision;            // BCD 리비전
} SMBIOS_ENTRY_POINT, * PSMBIOS_ENTRY_POINT;

// SMBIOS 헤더 (모든 SMBIOS 구조체의 공통 헤더)
typedef struct _SMBIOS_HEADER {
    UCHAR Type;      // 구조체 타입 (0=BIOS, 1=System, 2=Baseboard, 4=Processor 등)
    UCHAR Length;    // 구조체 길이
    USHORT Handle;   // 핸들
} SMBIOS_HEADER, * PSMBIOS_HEADER;

// SMBIOS Type 2: 베이스보드 정보
typedef struct _SMBIOS_BASEBOARD_INFO {
    SMBIOS_HEADER Header;
    UCHAR Manufacturer;                   // 제조사 문자열 인덱스
    UCHAR Product;                        // 제품명 문자열 인덱스
    UCHAR Version;                        // 버전 문자열 인덱스
    UCHAR SerialNumber;                   // 시리얼 번호 문자열 인덱스
    UCHAR AssetTag;                       // 자산 태그 문자열 인덱스
    UCHAR FeatureFlags;                   // 기능 플래그
    UCHAR LocationInChassis;              // 섀시 내 위치 문자열 인덱스
    USHORT ChassisHandle;                 // 섀시 핸들
    UCHAR BoardType;                      // 보드 타입
    UCHAR NumberOfContainedObjectHandles; // 포함된 객체 핸들 수
} SMBIOS_BASEBOARD_INFO, * PSMBIOS_BASEBOARD_INFO;

// SMBIOS Type 1: 시스템 정보
typedef struct _SMBIOS_SYSTEM_INFO {
    SMBIOS_HEADER Header;
    UCHAR Manufacturer;   // 제조사 문자열 인덱스
    UCHAR ProductName;    // 제품명 문자열 인덱스
    UCHAR Version;        // 버전 문자열 인덱스
    UCHAR SerialNumber;   // 시리얼 번호 문자열 인덱스
    UCHAR UUID[16];       // 시스템 UUID (하드웨어 지문으로 사용)
    UCHAR WakeupType;     // 웨이크업 타입
} SMBIOS_SYSTEM_INFO, * PSMBIOS_SYSTEM_INFO;

// SMBIOS Type 4: 프로세서 정보
typedef struct _SMBIOS_PROCESSOR_INFO {
    SMBIOS_HEADER Header;
    UCHAR SocketDesignation;        // 소켓 지정 문자열 인덱스
    UCHAR ProcessorType;            // 프로세서 타입
    UCHAR ProcessorFamily;          // 프로세서 패밀리
    UCHAR ProcessorManufacturer;    // 제조사 문자열 인덱스
    UCHAR ProcessorID[8];           // 프로세서 ID (CPU 시리얼)
    UCHAR ProcessorVersion;         // 프로세서 버전 문자열 인덱스
    UCHAR Voltage;                  // 전압
    USHORT ExternalClock;           // 외부 클럭
    USHORT MaxSpeed;                // 최대 속도
    USHORT CurrentSpeed;            // 현재 속도
    UCHAR Status;                   // 상태
    UCHAR ProcessorUpgrade;         // 프로세서 업그레이드
    USHORT L1CacheHandle;           // L1 캐시 핸들
    USHORT L2CacheHandle;           // L2 캐시 핸들
    USHORT L3CacheHandle;           // L3 캐시 핸들
    UCHAR SerialNumber;             // 시리얼 번호 문자열 인덱스
    UCHAR AssetTag;                 // 자산 태그 문자열 인덱스
    UCHAR PartNumber;               // 파트 번호 문자열 인덱스
    UCHAR CoreCount;                // 코어 수
    UCHAR CoreEnabled;              // 활성화된 코어 수
    UCHAR ThreadCount;              // 스레드 수
    USHORT ProcessorCharacteristics; // 프로세서 특성
} SMBIOS_PROCESSOR_INFO, * PSMBIOS_PROCESSOR_INFO;

// SMBIOS Type 17: 메모리 디바이스 정보
typedef struct _SMBIOS_MEMORY_DEVICE {
    SMBIOS_HEADER Header;
    USHORT PhysicalMemoryArrayHandle; // 물리 메모리 배열 핸들
    USHORT MemoryErrorInfoHandle;     // 메모리 에러 정보 핸들
    USHORT TotalWidth;                // 전체 너비
    USHORT DataWidth;                 // 데이터 너비
    USHORT Size;                      // 크기 (MB)
    UCHAR FormFactor;                 // 폼 팩터
    UCHAR DeviceSet;                  // 디바이스 세트
    UCHAR DeviceLocator;              // 디바이스 위치 문자열 인덱스
    UCHAR MemoryType;                 // 메모리 타입
    USHORT TypeDetail;                // 타입 세부 정보
    USHORT Speed;                     // 속도 (MHz)
    UCHAR Manufacturer;               // 제조사 문자열 인덱스
    UCHAR SerialNumber;               // 시리얼 번호 문자열 인덱스
    UCHAR AssetTag;                   // 자산 태그 문자열 인덱스
    UCHAR PartNumber;                 // 파트 번호 문자열 인덱스
} SMBIOS_MEMORY_DEVICE, * PSMBIOS_MEMORY_DEVICE;

// 하드웨어 정보 요청 구조체
typedef struct _HARDWARE_REQUEST {
    ULONG RequestType;  // 요청 타입 (HW_REQUEST_*)
    ULONG Reserved;     // 예약
} HARDWARE_REQUEST, * PHARDWARE_REQUEST;