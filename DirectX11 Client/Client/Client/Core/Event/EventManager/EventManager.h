// 이벤트 관리자 (모듈 간 느슨한 결합을 위한 이벤트 기반 통신 시스템)
// Observer 패턴으로 각 모듈이 직접 의존하지 않고 이벤트를 통해 통신
#pragma once
#include<functional>
#include <vector>
#include <memory>
#include <unordered_map>
#include <thread>

enum class EventType : uint8_t;  // 이벤트 타입 열거형
using QWORD = unsigned long long;  // 64비트 함수 포인터용

class EventManager
{
private:
	// 콜백 함수 정보 구조체
	struct callback
	{
		QWORD* funcPtr;  // 멤버 함수 포인터 (QWORD로 변환)
		void* thisPtr;   // 객체 인스턴스 포인터 (멤버 함수 호출 시 필요)
	};

	// 이벤트 타입별 콜백 함수 맵 (O(1) 조회)
	std::unordered_map<EventType, callback> callbacks;

private:
	// SEH를 적용한 안전한 이벤트 실행 (예외 발생 시 프로그램 크래시 방지)
	void trigger_seh(EventType type, const std::function<void()>& fn);

public:
	EventManager();
	~EventManager();

	// 이벤트 타입에 콜백 함수 등록
	void add_event(EventType type, void* thisPtr, QWORD* func);

	// 이벤트 트리거 (가변 인자 템플릿)
	// wait: true이면 동기 실행(join), false이면 비동기 실행(detach)
	template <typename... Args>
	void trigger(EventType type, bool wait, const Args&... args)
	{
		// 이벤트 타입이 등록되어 있는지 확인
		auto it = callbacks.find(type);
		if (it != callbacks.end())
		{
			auto& cb = it->second;

			// 새로운 스레드에서 콜백 함수 실행
			std::thread triggerthread([this, cb, type, args...]
				{
					// SEH를 적용하여 안전하게 실행
					trigger_seh(type, [cb, &args...]()
						{
							// 멤버 함수 포인터를 실제 함수 타입으로 변환
							using callFunc = void(__thiscall*)(void*, Args...);
							auto func = reinterpret_cast<callFunc>(cb.funcPtr);

							// 멤버 함수 호출 (this 포인터 + 인자들)
							func(cb.thisPtr, args...);
						});
				});

			// 동기/비동기 선택
			if (wait)
			{
				triggerthread.join();  // 스레드 종료 대기 (동기)
			}
			else
			{
				triggerthread.detach();  // 스레드 독립 실행 (비동기)
			}
		}
	}
};