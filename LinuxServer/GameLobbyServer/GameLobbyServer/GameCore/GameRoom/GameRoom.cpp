#include "GameRoom.h"

GameRoom::GameRoom(unsigned long long in_mapid) : mapid(in_mapid) {}

GameRoom::~GameRoom() {}

// 게임 룸 초기화
void GameRoom::init_GameRoom() { playerHashes.clear(); }

// 플레이어 추가
bool GameRoom::addPlayerHash(const std::string& hash) {
    std::lock_guard<std::mutex> lock(playerHashesMutex);

    // 중복 체크
    if (playerHashes.find(hash) != playerHashes.end()) {
        return false;
    }

    // 정원 초과 체크
    if (playerHashes.size() >= maxplayer) {
        return false;
    }

    playerHashes.insert(hash);
    return true;
}

// 플레이어 제거
bool GameRoom::removePlayerHash(const std::string& hash) {
    std::lock_guard<std::mutex> lock(playerHashesMutex);
    auto it = playerHashes.find(hash);
    if (it != playerHashes.end()) {
        playerHashes.erase(it);
        return true;
    }
    return false;
}

// 플레이어 존재 여부 확인
bool GameRoom::hasPlayerHash(const std::string& hash) {
    std::lock_guard<std::mutex> lock(playerHashesMutex);
    return playerHashes.find(hash) != playerHashes.end();
}

// 모든 플레이어 해시 조회
std::unordered_set<std::string> GameRoom::GetALLplayerHashes()
{
    std::lock_guard<std::mutex> lock(playerHashesMutex);
    return playerHashes;
}

// 룸이 가득 찼는지 확인
bool GameRoom::isFull() {
    std::lock_guard<std::mutex> lock(playerHashesMutex);
    return playerHashes.size() >= maxplayer;
}

// 현재 플레이어 수 조회
size_t GameRoom::getPlayerCount() {
    std::lock_guard<std::mutex> lock(playerHashesMutex);
    return playerHashes.size();
}

// 맵 ID 조회
unsigned long long GameRoom::getMapId() { return mapid; }