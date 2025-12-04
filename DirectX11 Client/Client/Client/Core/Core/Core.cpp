// 코어 시스템 구현부
#include "Core.h"
#include "../gui/window/window.h"
#include "../Network/Network/NetWork.h"
#include "../gui/guicontrol/GuiControl.h"
#include"../Network/Packet/PacketStructure/PacketStructure.h"
#include "../ProcessManager/ProcessManager.h"
#include "../Network/Packet/RoutineProgress/RoutineProgress.h"
#include "../Event/ProcessHandler/ProcessHandler.h"
#include "../Event/EventManager/EventManager.h"
#include <functional>
#include <Windows.h>
#include "../Event/EventType/EventType.h"
#include "../AntiCheat/AntiCheat.h"

// 전역 싱글톤 인스턴스 생성
std::unique_ptr<Core> G_core = std::make_unique<Core>();

// 생성자: EventManager 및 window 생성
Core::Core()
{
	C_eventmanager = std::make_unique<EventManager>();
	C_window = std::make_unique<window>();
}

Core::~Core()
{
}

// EventManager 인스턴스 반환
EventManager* Core::get_C_eventmanager()
{
	return C_eventmanager.get();
}

// 모든 시스템 초기화
bool Core::Init()
{
	// EventManager 이벤트 등록
	if (!Init_EventManager())return false;

	// 네트워크 초기화 및 Routine 서버 연결
	if (!Nework_Init())return false;

	return true;
}

// GUI 업데이트 루프 시작
void Core::Update(HINSTANCE hInstance)
{
	// GUI 스레드 생성 및 실행
	GuiThread = std::thread(std::bind(&window::Update, C_window.get(), hInstance));

	// GUI 스레드 종료까지 대기
	GuiThread.join();
}

// EventManager에 모든 이벤트 등록
bool Core::Init_EventManager()
{
	// 필수 컴포넌트 유효성 검사
	if (!C_eventmanager || !G_network || !C_window)return false;

	// 메시지 이벤트 등록 (MessageBox 표시)
	if (!RegisterEvent(EventType::MESSAGE_EVENT, &ProcessHandler::MsgHandler))return false;

	// 네트워크 연결 이벤트 등록
	if (!RegisterEvent(EventType::SUCESS_ROUTINEAUTH, &NetWork::ConnectToAuthServer))return false;  
	if (!RegisterEvent(EventType::SUCESS_GAMELOBBY, &NetWork::ConnectToGameLobbyServer))return false; 

	// 패킷 전송 이벤트 등록
	if (!RegisterEvent(EventType::TwoStringPacket_EVNET, &RoutineProgress::SendResponseForTwoStringPacket))return false;  
	if (!RegisterEvent(EventType::HashPacket_EVNET, &RoutineProgress::SendResponseForHashPacket))return false;  

	// GUI 전환 이벤트 등록
	if (!RegisterEvent(EventType::CHANGE_UI_TYPE, &GuiControl::SetUitype))return false;

	// 프로세스 제어 이벤트 등록
	if (!RegisterEvent(EventType::TERMINATE_PROCESSEVENT, &Core::Release))return false; 
	if (!RegisterEvent(EventType::STARTGAME_EVENT, &ProcessManager::ProcessRunner))return false;  

	// 안티치트 이벤트 등록 (G_AntiCheat가 존재할 경우)
	if (G_AntiCheat)
	{
		if (!RegisterEvent(EventType::SECURITY_Init_EVENT, &AntiCheat::Start))return false;  
		if (!RegisterEvent(EventType::HWID_DATA_EVENT, &AntiCheat::RequestHardwareInfo))return false;  
		if (!RegisterEvent(EventType::SECURITY_Heartbeat_EVENT, &AntiCheat::ServerCheckLogic))return false;  
	}
	return true;
}

// 네트워크 초기화 및 Routine 서버 연결
bool Core::Nework_Init()
{
	// NetWork 클래스 초기화 (Winsock, AES 키 등)
	if (!G_network->Initialize())
	{
		return false;
	}

	// Routine 인증 서버 연결 (Auth 서버 정보 획득용)
	if (!G_network->ConnectToRoutinAuthServer())
	{
		// 연결 실패 시 네트워크 정리
		G_network->CleanUp();
		return false;
	}


	return true;
}

// 리소스 정리 및 애플리케이션 종료
void Core::Release()
{
	// 네트워크 연결 종료 및 소켓 정리
	G_network->CleanUp();
}

// 이벤트 타입과 멤버 함수를 EventManager에 등록
template<typename Func>
bool Core::RegisterEvent(EventType type, Func memberFunc)
{

	QWORD funcAddr = *reinterpret_cast<QWORD*>(&memberFunc);
	void* thisptr = nullptr;


	if (!funcAddr)return false;

	// 이벤트 타입에 따라 적절한 객체 인스턴스 선택
	if (type >= EventType::PRIORITY_PACKET && type <= EventType::SUCESS_GAMELOBBY)
	{
		// 네트워크 연결 관련 이벤트 -> NetWork 클래스
		thisptr = G_network.get();
	}
	else if (type >= EventType::TypePacketREQUEST_EVNET && type <= EventType::CANCLEROOM_EVENT)
	{
		// 패킷 전송 관련 이벤트 -> NetWork 클래스
		thisptr = G_network.get();
	}
	else if (type == EventType::MESSAGE_EVENT || type == EventType::Exception_Error)
	{
		// 메시지 및 예외 처리 이벤트 -> ProcessHandler 클래스
		thisptr = G_ProcessHandler.get();
	}
	else if (type == EventType::CHANGE_UI_TYPE)
	{
		// GUI 전환 이벤트 -> GuiControl 클래스
		thisptr = G_GuiControl.get();
	}
	else if (type == EventType::TERMINATE_PROCESSEVENT)
	{
		// 애플리케이션 종료 이벤트 -> Core 클래스 (자기 자신)
		thisptr = G_core.get();
	}
	else if (type == EventType::SECURITY_Init_EVENT ||
		type == EventType::HWID_DATA_EVENT ||
		type == EventType::SECURITY_Heartbeat_EVENT)
	{
		// 안티치트 관련 이벤트 -> AntiCheat 클래스
		thisptr = G_AntiCheat.get();
	}
	else if (type == EventType::STARTGAME_EVENT)
	{
		// 게임 프로세스 실행 이벤트 -> ProcessManager 클래스
		thisptr = G_ProcessManager.get();
	}
	else
	{
		// 알 수 없는 이벤트 타입
		return false;
	}

	// EventManager에 이벤트 등록
	C_eventmanager->add_event(type, thisptr, reinterpret_cast<QWORD*>(funcAddr));
	return true;
}

// 명시적 템플릿 인스턴스화 (링커 오류 방지)
template bool Core::RegisterEvent<void(NetWork::*)(const std::string&, const std::string&, const std::string&)>(EventType, void(NetWork::*)(const std::string&, const std::string&, const std::string&));