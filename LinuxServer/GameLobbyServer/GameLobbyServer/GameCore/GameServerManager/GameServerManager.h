#ifndef GAMESERVERMANAGER_H
#define GAMESERVERMANAGER_H
#pragma once
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <queue>

class Player;
class GameRoom;
enum class ResultType : uint8_t;

// 하트비트 우선순위 큐 비교자
struct PlayerHeartbeatComparator {
    bool operator()(Player* a, Player* b) const;
};

// 게임 로비 및 매칭 시스템 관리자
class GameServerManager {
public:
    GameServerManager();
    ~GameServerManager();

    // 플레이어 관리
    bool DeletePlayer(const std::string& args);
    void HandleLoginAttempt(int sock, const std::string& hash);
    Player* FindPlayer(const std::string& hash);

    // 매칭 시스템
    bool joinOrCreateRoom(const std::string& hash, const std::string& nickname);
    void SendGameStart(const std::unordered_set<std::string>& in_playerhashes);
    bool CancleRoom(const std::string& hash);

    // 유틸리티
    std::string GetsocketToHash(int sock);

    // 안티치트 하트비트
    void ResponseHeartbeat(int sock, ResultType result);
    void Heartbeat(std::vector<uint8_t>* paket_data);

private:
    void RegisterPlayer(int sock, const std::string& hash);
    void HandleExistingPlayer(Player* player, int sock, const std::string& hash);
    void CleanINLOBBY();
    void INROOM();
    void ErasePlayerAndSocket(Player* player, const std::string& args, int Sock);
    bool RemovePlayerFromRoomAndCleanup(GameRoom* room, const std::string& args);

private:
    std::unordered_map<std::string, Player*> players; // 전체 플레이어 목록
    std::unordered_map<std::string, Player*> heatbeatforplayer; // 하트비트 관리용
    std::unordered_map<int, std::string> socketToHash; // 소켓 -> 세션 토큰 매핑
    std::unordered_set<GameRoom*> waitrooms; // 대기 중인 룸
    std::unordered_set<GameRoom*> startedrooms; // 게임 시작된 룸

    std::mutex players_MTX;
    std::mutex heatbeatforplayer_MTX;
    std::mutex socketToHash_MTX;
    std::mutex waitrooms_MTX;
    std::mutex startedrooms_MTX;
};

extern std::unique_ptr<GameServerManager> G_gameservermanager;
#endif