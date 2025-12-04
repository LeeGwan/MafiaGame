// 코어 시스템 (전체 시스템 초기화 및 생명주기 관리)
#pragma once

#include <memory>
#include <thread>

using QWORD = unsigned long long; 
enum class EventType : uint8_t;   
class NetWork;                    
class window;                     
class EventManager;                
class RoutineProgress;            
class AntiCheat;                 
using HINSTANCE = struct HINSTANCE__*;  

class Core
{
public:
	Core();
	~Core();

	// EventManager 인스턴스 반환
	EventManager* get_C_eventmanager();

	// 모든 시스템 초기화 (EventManager 설정, 네트워크 초기화)
	bool Init();

	// GUI 업데이트 루프 시작 (메인 스레드는 GUI 스레드 종료까지 대기)
	void Update(HINSTANCE hInstance);

	// 리소스 정리 및 애플리케이션 종료
	void Release();

private:
	// EventManager에 모든 이벤트 등록
	bool Init_EventManager();

	// 네트워크 초기화 및 Routine 서버 연결
	bool Nework_Init();

	// 이벤트 타입과 멤버 함수를 EventManager에 등록 (템플릿)
	template<typename Func>
	bool RegisterEvent(EventType type, Func memberFunc);

private:
	std::unique_ptr<EventManager> C_eventmanager;  // 이벤트 관리자
	std::unique_ptr<window> C_window;              // GUI 윈도우
	std::thread GuiThread;                         // GUI 메시지 루프 스레드
};

// 전역 싱글톤 인스턴스
extern std::unique_ptr<Core> G_core;