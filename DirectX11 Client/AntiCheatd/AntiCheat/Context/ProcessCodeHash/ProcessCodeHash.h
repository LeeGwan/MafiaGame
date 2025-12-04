// 프로세스 코드 무결성 검증 관련 구조체
// PE 헤더 파싱 및 CRC32 기반 메모리 검증
#pragma once
#include <ntddk.h>

#define CODE_HASH_SIZE 0x1000  // 4096 bytes

// PE DOS 헤더 (MZ 시그니처)
typedef struct _IMAGE_DOS_HEADER {
    USHORT e_magic;      // "MZ" (0x5A4D)
    USHORT e_cblp;
    USHORT e_cp;
    USHORT e_crlc;
    USHORT e_cparhdr;
    USHORT e_minalloc;
    USHORT e_maxalloc;
    USHORT e_ss;
    USHORT e_sp;
    USHORT e_csum;
    USHORT e_ip;
    USHORT e_cs;
    USHORT e_lfarlc;
    USHORT e_ovno;
    USHORT e_res[4];
    USHORT e_oemid;
    USHORT e_oeminfo;
    USHORT e_res2[10];
    LONG e_lfanew;       // PE 헤더 오프셋
} IMAGE_DOS_HEADER, * PIMAGE_DOS_HEADER;

// PE 파일 헤더
typedef struct _IMAGE_FILE_HEADER {
    USHORT Machine;              // CPU 아키텍처
    USHORT NumberOfSections;     // 섹션 개수
    ULONG TimeDateStamp;         // 타임스탬프
    ULONG PointerToSymbolTable;  // 심볼 테이블 포인터
    ULONG NumberOfSymbols;       // 심볼 개수
    USHORT SizeOfOptionalHeader; // Optional 헤더 크기
    USHORT Characteristics;      // 특성
} IMAGE_FILE_HEADER, * PIMAGE_FILE_HEADER;

// 데이터 디렉토리
typedef struct _IMAGE_DATA_DIRECTORY {
    ULONG VirtualAddress;  // RVA
    ULONG Size;            // 크기
} IMAGE_DATA_DIRECTORY, * PIMAGE_DATA_DIRECTORY;

// PE Optional 헤더 (64비트)
typedef struct _IMAGE_OPTIONAL_HEADER64 {
    USHORT Magic;                        // 0x20B (PE32+)
    UCHAR MajorLinkerVersion;
    UCHAR MinorLinkerVersion;
    ULONG SizeOfCode;                    // 코드 섹션 크기
    ULONG SizeOfInitializedData;
    ULONG SizeOfUninitializedData;
    ULONG AddressOfEntryPoint;           // 진입점 RVA
    ULONG BaseOfCode;                    // 코드 베이스 RVA
    ULONGLONG ImageBase;                 // 이미지 베이스 주소
    ULONG SectionAlignment;
    ULONG FileAlignment;
    USHORT MajorOperatingSystemVersion;
    USHORT MinorOperatingSystemVersion;
    USHORT MajorImageVersion;
    USHORT MinorImageVersion;
    USHORT MajorSubsystemVersion;
    USHORT MinorSubsystemVersion;
    ULONG Win32VersionValue;
    ULONG SizeOfImage;                   // 이미지 전체 크기
    ULONG SizeOfHeaders;                 // 헤더 크기
    ULONG CheckSum;
    USHORT Subsystem;
    USHORT DllCharacteristics;
    ULONGLONG SizeOfStackReserve;
    ULONGLONG SizeOfStackCommit;
    ULONGLONG SizeOfHeapReserve;
    ULONGLONG SizeOfHeapCommit;
    ULONG LoaderFlags;
    ULONG NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[16];  // 데이터 디렉토리 배열
} IMAGE_OPTIONAL_HEADER64, * PIMAGE_OPTIONAL_HEADER64;

// PE NT 헤더 (PE 시그니처 + 파일 헤더 + Optional 헤더)
typedef struct _IMAGE_NT_HEADERS64 {
    ULONG Signature;                      // "PE\0\0" (0x00004550)
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER64 OptionalHeader;
} IMAGE_NT_HEADERS64, * PIMAGE_NT_HEADERS64;

#define IMAGE_SIZEOF_SHORT_NAME 8

// PE 섹션 헤더
typedef struct _IMAGE_SECTION_HEADER {
    UCHAR Name[IMAGE_SIZEOF_SHORT_NAME];  // 섹션 이름 (".text", ".rdata" 등)
    union {
        ULONG PhysicalAddress;
        ULONG VirtualSize;                // 메모리에서의 섹션 크기
    } Misc;
    ULONG VirtualAddress;                 // 섹션 RVA
    ULONG SizeOfRawData;                  // 파일에서의 섹션 크기
    ULONG PointerToRawData;               // 파일 오프셋
    ULONG PointerToRelocations;
    ULONG PointerToLinenumbers;
    USHORT NumberOfRelocations;
    USHORT NumberOfLinenumbers;
    ULONG Characteristics;                // 섹션 특성 (실행, 읽기, 쓰기 등)
} IMAGE_SECTION_HEADER, * PIMAGE_SECTION_HEADER;

#define IMAGE_DOS_SIGNATURE 0x5A4D     // MZ
#define IMAGE_NT_SIGNATURE  0x00004550 // PE00

// CRC32 기반 코드 무결성 검증 정의
#define CODE_CHUNK_SIZE 0x100  // 256바이트 단위로 코드 분할

// KAPC_STATE 구조체 정의
#ifndef _KAPC_STATE_DEFINED
#define _KAPC_STATE_DEFINED
typedef struct _KAPC_STATE {
    LIST_ENTRY ApcListHead[2];
    PKPROCESS Process;
    BOOLEAN KernelApcInProgress;
    BOOLEAN KernelApcPending;
    BOOLEAN UserApcPending;
} KAPC_STATE, * PKAPC_STATE;
#endif

// PEB 구조체 정의 (간소화 버전)
#ifndef _PEB_DEFINED
#define _PEB_DEFINED
typedef struct _PEB {
    UCHAR InheritedAddressSpace;
    UCHAR ReadImageFileExecOptions;
    UCHAR BeingDebugged;
    UCHAR BitField;
    PVOID Mutant;
    PVOID ImageBaseAddress;  // 오프셋 0x10 (메인 모듈 베이스 주소)
    // 나머지 필드 생략
} PEB, * PPEB;
#endif

// 코드 섹션 정보 (.text, .rdata)
typedef struct _CODE_SECTION_INFO {
    PVOID Address;  // 섹션 시작 주소
    ULONG Size;     // 섹션 크기
} CODE_SECTION_INFO, * PCODE_SECTION_INFO;

// 프로세스 코드 해시 정보 (CRC32 기반)
typedef struct _PROCESS_CODE_HASH {
    HANDLE ProcessId;                  // 프로세스 ID
    PVOID CodeBaseAddress;             // 코드 베이스 주소
    ULONG CodeSize;                    // 전체 코드 크기
    ULONG ChunkCount;                  // 청크 개수 (256바이트 단위)
    PULONG HashValues;                 // CRC32 해시 배열 (동적 할당)
    ULONG SectionCount;                // 섹션 개수 (.text, .rdata)
    CODE_SECTION_INFO Sections[2];     // 섹션 정보 배열
    BOOLEAN IsValid;                   // 유효성 플래그
} PROCESS_CODE_HASH, * PPROCESS_CODE_HASH;

// 보안 경고 구조체 (공격 시도 로그)
typedef struct _SECURITY_ALERT {
    HANDLE AttackerPID;        // 공격자 프로세스 ID
    HANDLE TargetPID;          // 공격 대상 프로세스 ID
    CHAR AttackerName[16];     // 공격자 프로세스 이름
    LARGE_INTEGER Timestamp;   // 공격 시도 시간
    ACCESS_MASK AttemptedAccess;  // 시도한 접근 권한
} SECURITY_ALERT, * PSECURITY_ALERT;

// 보호 대상 프로세스 정보
typedef struct _PROCESS_PROTECTED {
    HANDLE ProcessId;  // 프로세스 ID
    BOOLEAN IsValid;   // 유효성 플래그
} PROCESS_PROTECTED, * PPROCESS_PROTECTED;

// 프로세스 보호 요청 구조체
typedef struct _PROTECTION_REQUEST {
    ULONG RequestType;  // 요청 타입 (SECURITY_REQUEST_*)
    HANDLE ProcessId;   // 대상 프로세스 ID
    ULONG Reserved;     // 예약
} PROTECTION_REQUEST, * PPROTECTION_REQUEST;

//프로세스 파일명 가져오기
NTKERNELAPI
PCHAR
PsGetProcessImageFileName(
    _In_ PEPROCESS Process
);
//PEPROCESS 가져오기
NTKERNELAPI
NTSTATUS
PsLookupProcessByProcessId(
    _In_ HANDLE ProcessId,
    _Outptr_ PEPROCESS* Process
);