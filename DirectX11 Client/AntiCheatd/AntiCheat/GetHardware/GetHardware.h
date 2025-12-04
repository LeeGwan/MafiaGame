// 하드웨어 정보 수집 함수 선언
// SMBIOS 파싱을 통한 메인보드 UUID 및 CPU ID 수집
#pragma once
#include <ntddk.h>

// 메인보드 시리얼 번호 (UUID) 가져오기
NTSTATUS GetMainboardSerial(PCHAR Buffer, ULONG BufferSize);

// CPU 정보 (Processor ID) 가져오기
NTSTATUS GetCPUInfo(PCHAR Buffer, ULONG BufferSize);


// SMBIOS 문자열 가져오기
PCHAR GetSMBIOSString(PUCHAR StringPtr, UCHAR StringIndex);

// 하드웨어 정보 응답 메시지 생성 (TLV 구조)
NTSTATUS BuildHardwareResponseMessage(ULONG RequestType, PUCHAR* Message, PULONG MessageSize);