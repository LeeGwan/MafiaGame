#ifndef GAMEROOM_H
#define GAMEROOM_H

#pragma once
#include <mutex>
#include <string>
#include <unordered_set>

// 매칭 룸 관리 클래스
class GameRoom
{
public:
    GameRoom(unsigned long long in_mapid = 0);
    ~GameRoom();
    void init_GameRoom();

    // 룸 정보 조회
    unsigned long long getMapId();
    size_t getPlayerCount();
    bool isFull();

    // 플레이어 관리
    bool addPlayerHash(const std::string& hash);
    bool removePlayerHash(const std::string& hash);
    bool hasPlayerHash(const std::string& hash);
    std::unordered_set<std::string> GetALLplayerHashes();

private:
    unsigned long long mapid;
    const size_t maxplayer = 6; // 최대 플레이어 수 (매칭 기준)
    std::unordered_set<std::string> playerHashes; // 플레이어 세션 토큰 목록
    std::mutex playerHashesMutex;
};

#endif