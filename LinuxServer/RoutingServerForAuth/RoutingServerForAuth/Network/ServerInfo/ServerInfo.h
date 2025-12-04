#ifndef SERVERINFO_H
#define SERVERINFO_H

#pragma once
#include <string>
#include <chrono>
#include <atomic>
#include <memory>

// 게임 서버 정보 및 상태 관리
class ServerInfo
{
public:
    ServerInfo();
    ~ServerInfo();

    // 서버 정보 등록
    void RegisterServerInfo(const std::string& in_ip, uint16_t in_port);

    // 하트비트 관련
    std::chrono::steady_clock::time_point Get_lastHeartbeat()const;
    void Set_lastHeartbeat(const std::chrono::steady_clock::time_point& in_time);
    bool IsSendHeartbeat();
    void SetsendHeartbeat(bool SetsendHeartbeat);

    // 서버 정보 조회
    std::pair<std::string, uint16_t> Get_serverInfor();

    // 서버 사용 가능 여부
    bool Isused();
    void Setused(bool Setused);

private:
    std::string Ip;
    uint16_t port;
    std::chrono::steady_clock::time_point lastHeartbeat; // 마지막 하트비트 시간
    std::atomic<bool> sendHeartbeat; // 하트비트 전송 플래그
    std::atomic<bool> used; // 서버 활성화 상태
};

#endif