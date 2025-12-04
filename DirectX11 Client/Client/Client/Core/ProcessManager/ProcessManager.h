// 언리얼 게임 프로세스 실행 관리자
#pragma once
#include <memory>
#include <string>
#include <cstdint>
class ProcessManager
{
public:
	// 언리얼 게임 실행 (IP, 포트, 세션 토큰 전달)
	void ProcessRunner(const std::string& IP, uint16_t port);
private:

};

extern std::unique_ptr<ProcessManager> G_ProcessManager;