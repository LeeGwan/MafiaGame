// 이벤트 타입 정의 (EventManager에서 사용)
// 각 모듈 간 통신을 위한 이벤트 타입 열거형
#pragma once
#include <cstdint>

enum class EventType : uint8_t
{
	// 메시지 박스 표시 이벤트
	MESSAGE_EVENT = 0x0,  // ProcessHandler::MsgHandler 호출

	// 네트워크 연결 이벤트
	PRIORITY_PACKET,       // 우선순위 패킷 전송 (즉시 처리)
	SUCESS_ROUTINEAUTH,    // Routine 서버 응답 후 Auth 서버 연결
	SUCESS_AUTH,           // Auth 서버 인증 성공
	SUCESS_GAMELOBBY,      // 로그인 성공 후 게임 로비 서버 연결

	// 패킷 전송 이벤트
	TypePacketREQUEST_EVNET,   // TypePacket 전송 (타입만 포함)
	TwoStringPacket_EVNET,      // TwoStringPacket 전송 (로그인/회원가입)
	HashPacket_EVNET,           // HashPacket 전송 (세션 해시)
	ResultPacket_EVENT,         // ResultPacket 전송 (결과 코드)
	LOGOUT_EVENT,               // 로그아웃 요청
	CHECKLOBBY_EVENT,           // 로비 서버 세션 검증
	JOINROOM_EVEVNT,            // 게임 방 참가
	CANCLEROOM_EVENT,           // 게임 방 취소
	STARTGAME_EVENT,            // 게임 시작 (게임 서버 연결)

	// GUI 제어 이벤트
	CHANGE_UI_TYPE,  // UI 화면 전환 (Login, Lobby, Matching, Game)

	// 프로세스 제어 이벤트
	TERMINATE_PROCESSEVENT,  // 애플리케이션 종료

	// 안티치트 이벤트
	SECURITY_Init_EVENT,       // 안티치트 시스템 초기화 (드라이버 로드)
	HWID_DATA_EVENT,           // 하드웨어 ID 수집 및 서버 전송
	SECURITY_Heartbeat_EVENT,  // 안티치트 하트비트 응답

	// 에러 처리 이벤트
	Exception_Error,  // SEH 예외 발생 시 처리

	EVENTSIZE = 0x40,  // 이벤트 배열 최대 크기
};