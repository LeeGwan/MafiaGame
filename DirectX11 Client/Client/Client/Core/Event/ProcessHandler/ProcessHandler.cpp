// 프로세스 예외 처리 구현부
#include "ProcessHandler.h"
#include <fstream>
#include "../EventManager/EventManager.h"
#include"../EventType/EventType.h"
#include "../../Network/Packet/PacketStructure/PacketStructure.h"

// 전역 싱글톤 인스턴스 생성
std::unique_ptr<ProcessHandler> G_ProcessHandler = std::make_unique<ProcessHandler>();

// 생성자: 전역 예외 핸들러 등록
ProcessHandler::ProcessHandler()
{
	// SetUnhandledExceptionFilter로 커스텀 예외 핸들러 등록
	// 처리되지 않은 예외 발생 시 MyUnhandledExceptionFilter 자동 호출
	SetUnhandledExceptionFilter(ProcessHandler::MyUnhandledExceptionFilter);
}

ProcessHandler::~ProcessHandler() = default;

// 전역 예외 핸들러 콜백 함수
LONG __stdcall ProcessHandler::MyUnhandledExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo)
{
	// 예외 발생 시 이벤트 트리거 (현재 주석 처리)
	//G_core->get_C_eventmanager()->trigger(EventType::Exception_Error, true);

	// 사용자에게 예외 발생 알림 (테스트용)
	MessageBoxA(NULL, "error", "", 0);

	// 예외 처리 완료 반환
	return EXCEPTION_EXECUTE_HANDLER;
}

// 서버 응답 결과를 MessageBox로 표시
void ProcessHandler::MsgHandler(ResultType result)
{
	// 성공 여부 판단 (회원가입, 로그인, 세션 검증 성공)
	bool successed = (result == ResultType::SignUp_Succeeded ||
		result == ResultType::Login_Succeeded ||
		result == ResultType::CheckSession_Succeeded);

	// ResultType을 문자열로 변환
	std::string msg = ConversationResult(result);

	// MessageBox 표시 (성공: 정보 아이콘, 실패: 경고 아이콘)
	MessageBoxA(NULL, msg.c_str(), successed ? "SUCESSED" : "FAILED",
		MB_OK | successed ? MB_ICONINFORMATION : MB_ICONWARNING);
}

// ResultType을 문자열로 변환
std::string ProcessHandler::ConversationResult(ResultType result)
{
	std::string resultstring;
	switch (result)
	{
		// 회원가입 결과
	case ResultType::SignUp_Failed:
		resultstring = "SignUp_Failed";
		break;
	case ResultType::SignUp_AlreadyExists:
		resultstring = "SignUp_AlreadyExists";  // 이미 존재하는 계정
		break;
	case ResultType::SignUp_Succeeded:
		resultstring = "SignUp_Succeeded";
		break;

		// 로그인 결과
	case ResultType::Login_Failed:
		resultstring = "Login_Failed";
		break;
	case ResultType::Login_InvalidCredentials:
		resultstring = "Login_InvalidCredentials";  // 잘못된 인증 정보
		break;
	case ResultType::Login_AlreadyLoggedIn:
		resultstring = "Login_AlreadyLoggedIn";  // 중복 로그인
		break;
	case ResultType::Login_Succeeded:
		resultstring = "Login_Succeeded";
		break;

		// 세션 검증 결과
	case ResultType::CheckSession_Succeeded:
		resultstring = "Successed To Join GameLobby.";
		break;
	case ResultType::CheckSession_Failed:
		resultstring = "Failed To Join GameLobby.";
		break;

	default:
		break;
	}
	return resultstring;
}

// SEH 예외 코드 생성
DWORD ProcessHandler::generate_exception()
{
	return EXCEPTION_EXECUTE_HANDLER;
}