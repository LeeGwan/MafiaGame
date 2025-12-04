// 프로세스 접근 권한 정의
// ObRegisterCallbacks에서 차단할 권한 플래그
#pragma once

// 프로세스 접근 권한 정의 (Windows API)
#define PROCESS_TERMINATE                  0x0001  // 프로세스 종료
#define PROCESS_CREATE_THREAD              0x0002  // 스레드 생성
#define PROCESS_VM_OPERATION               0x0008  // 가상 메모리 작업
#define PROCESS_VM_READ                    0x0010  // 메모리 읽기
#define PROCESS_VM_WRITE                   0x0020  // 메모리 쓰기
#define PROCESS_DUP_HANDLE                 0x0040  // 핸들 복제
#define PROCESS_CREATE_PROCESS             0x0080  // 프로세스 생성
#define PROCESS_SET_QUOTA                  0x0100  // 할당량 설정
#define PROCESS_SET_INFORMATION            0x0200  // 정보 설정
#define PROCESS_SUSPEND_RESUME             0x0800  // 일시 중지/재개

// 완전 보호 권한 (차단할 모든 권한)
// 이 권한들을 요청하면 접근 거부
#define PROTECT_FULL_ACCESS (PROCESS_TERMINATE | \
                             PROCESS_CREATE_THREAD | \
                             PROCESS_VM_OPERATION | \
                             PROCESS_VM_WRITE | \
                             PROCESS_DUP_HANDLE | \
                             PROCESS_SET_QUOTA | \
                             PROCESS_SET_INFORMATION | \
                             PROCESS_SUSPEND_RESUME)

// 스레드 보호 권한
#define PROTECT_THREAD_ACCESS (THREAD_SUSPEND_RESUME | \
                               THREAD_TERMINATE | \
                               THREAD_SET_CONTEXT | \
                               THREAD_SET_INFORMATION | \
                               THREAD_GET_CONTEXT)