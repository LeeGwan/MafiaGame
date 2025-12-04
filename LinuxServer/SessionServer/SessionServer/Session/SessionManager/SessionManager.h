#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#pragma once
#include <unordered_map>
#include <mutex>
#include <memory.h>
#include <string>
#include <vector>

enum class ResultType : uint8_t;
class ClientInfo;

// 세션 관리자 (로그인 상태 및 하드웨어 정보 관리)
class SessionManager
{
public:
    SessionManager();
    ~SessionManager();

    // 인증 서버용: 로그인 세션 생성 및 중복 로그인 체크
    ResultType getPlayerStateForAuthserver(const std::string& hash);

    // 게임 로비 서버용: 세션 검증 및 하드웨어 정보 등록 (안티치트)
    bool getPlayerStateForGameLobbyserver(const std::string& hash, const std::string& mainboard, const std::string& CPUID);

    // 로그아웃 처리 (세션 삭제)
    void LogOut(const std::string& hash);

private:
    std::unordered_map<std::string, ClientInfo*> clientCache; // 세션 토큰 -> 클라이언트 정보
    std::mutex Cache_MTX;
};

#endif