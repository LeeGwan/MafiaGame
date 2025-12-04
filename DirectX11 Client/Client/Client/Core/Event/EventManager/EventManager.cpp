// 이벤트 관리자 구현부
#include "EventManager.h"
#include"../ProcessHandler/ProcessHandler.h"
#include "../EventType/EventType.h"

// SEH를 적용한 안전한 이벤트 실행
void EventManager::trigger_seh(EventType type, const std::function<void()>& fn)
{
	__try {
		// 람다 함수 실행 (실제 콜백 함수 호출)
		fn();
	}
	__except (G_ProcessHandler->generate_exception())
	{
		// 예외 발생 시 여기로 진입
		// 프로그램 크래시 방지
	}
}

// 생성자: callbacks 맵 메모리 예약
EventManager::EventManager()
{
	// EVENTSIZE(0x40) 크기만큼 미리 메모리 예약 (동적 재할당 방지)
	callbacks.reserve(static_cast<size_t>(EventType::EVENTSIZE));
}

// 소멸자: callbacks 맵 정리
EventManager::~EventManager()
{
	callbacks.clear();
}

// 이벤트 타입에 콜백 함수 등록
void EventManager::add_event(EventType type, void* thisPtr, QWORD* func)
{
	// callbacks 맵에 {이벤트 타입: 콜백 정보} 저장
	// 이미 등록된 이벤트 타입이면 덮어쓰기
	callbacks[type] = { func ,thisPtr };
}