#include "ServerInfo.h"

ServerInfo::ServerInfo() {}

ServerInfo::~ServerInfo() {}

// 게임 서버 정보 등록 및 초기화
void ServerInfo::RegisterServerInfo(const std::string& in_ip, uint16_t in_port)
{
    Ip = in_ip;
    port = in_port;
    sendHeartbeat.store(false);
    used.store(true);
    // 하트비트 타이머 초기화
    lastHeartbeat = std::chrono::steady_clock::now();
}

// 마지막 하트비트 시간 조회
std::chrono::steady_clock::time_point ServerInfo::Get_lastHeartbeat() const {
    return lastHeartbeat;
}

// 하트비트 시간 갱신
void ServerInfo::Set_lastHeartbeat(
    const std::chrono::steady_clock::time_point& in_time) {
    lastHeartbeat = in_time;
    sendHeartbeat.store(false);
}

// 서버 정보 조회
std::pair<std::string, uint16_t> ServerInfo::Get_serverInfor() {
    return { Ip, port };
}

// 하트비트 전송 상태 확인
bool ServerInfo::IsSendHeartbeat() { return sendHeartbeat.load(); }

void ServerInfo::SetsendHeartbeat(bool SetsendHeartbeat) {
    sendHeartbeat.store(SetsendHeartbeat);
}

// 서버 사용 가능 여부 확인
bool ServerInfo::Isused()
{
    return used.load();
}

void ServerInfo::Setused(bool Setused)
{
    used.store(Setused);
}