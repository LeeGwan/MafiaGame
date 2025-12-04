// 프로세스 예외 처리 및 메시지 핸들러
// SEH(Structured Exception Handling) 기반 전역 예외 처리 및 서버 응답 결과 표시
#pragma once
#include<Windows.h>
#include <memory>
#include <string>

enum class ResultType : uint8_t;  // 서버 응답 결과 타입

class ProcessHandler
{
public:
	// 생성자에서 SetUnhandledExceptionFilter 등록
	ProcessHandler();
	~ProcessHandler();

	// SEH 전역 예외 핸들러 (static 멤버 함수여야 함)
	static LONG __stdcall MyUnhandledExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo);

	// 서버 응답 결과를 MessageBox로 표시 (성공/실패 아이콘 포함)
	void MsgHandler(ResultType result);

	// SEH 예외 코드 생성 (EventManager의 trigger_seh에서 사용)
	DWORD generate_exception();

private:
	// ResultType을 사용자 친화적인 문자열로 변환
	static std::string ConversationResult(ResultType result);
};

// 전역 싱글톤 인스턴스
extern std::unique_ptr<ProcessHandler> G_ProcessHandler;