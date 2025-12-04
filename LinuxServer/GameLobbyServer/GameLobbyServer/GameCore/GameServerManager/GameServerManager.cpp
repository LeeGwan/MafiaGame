#include "GameServerManager.h"
#include "../../GameCore/GameRoom/GameRoom.h"
#include "../../GameCore/Player/Player.h"
#include "../../MemoryPool/MemoryPool.h"
#include "../../Network/DedicatedManger/DedicatedManger.h"
#include "../../Network/Packet/PacketStructure/PacketStructure.h"
#include "../../Network/Packet/RoutineProgress/RoutineProgress.h"
#include <mutex>
#include <iostream>
#include <unistd.h>

std::unique_ptr<GameServerManager> G_gameservermanager =
std::make_unique<GameServerManager>();

// 하트비트 우선순위 큐 비교자 (오래된 하트비트 우선)
bool PlayerHeartbeatComparator::operator()(Player* a, Player* b) const {
    return a->Get_AnticheatHeartbeat() > b->Get_AnticheatHeartbeat();
}

GameServerManager::GameServerManager() {}

GameServerManager::~GameServerManager() {}

// 로그인 시도 처리
void GameServerManager::HandleLoginAttempt(int sock, const std::string& hash) {
    if (sock == -1)
        return;

    Player* checkplayer = FindPlayer(hash);
    if (!checkplayer) {
        RegisterPlayer(sock, hash);
    }
    else {
        HandleExistingPlayer(checkplayer, sock, hash);
    }
}

// 기존 플레이어 처리
void GameServerManager::HandleExistingPlayer(Player* player, int sock,
    const std::string& hash) {
}

// 소켓으로 세션 토큰 조회
std::string GameServerManager::GetsocketToHash(int sock) {
    std::string hash;
    {
        std::lock_guard<std::mutex> lock(socketToHash_MTX);
        auto find = socketToHash.find(sock);
        if (find != socketToHash.end()) {
            hash = find->second;
        }
    }
    return hash;
}

// 플레이어 등록
void GameServerManager::RegisterPlayer(int sock, const std::string& hash) {
    {
        std::lock_guard<std::mutex> lock(players_MTX);
        players[hash] = G_MemoryPool->acquire<Player>();
    }

    auto time = std::chrono::steady_clock::now();
    players[hash]->Setstatus(playerstatus::INLOBBY);
    players[hash]->Set_socket(sock);
    players[hash]->Set_AnticheatHeartbeat(time);

    {
        std::lock_guard<std::mutex> lock(heatbeatforplayer_MTX);
        heatbeatforplayer[hash] = players[hash];
    }
    {
        std::lock_guard<std::mutex> lock(socketToHash_MTX);
        socketToHash[sock] = hash;
    }
}

// 플레이어 및 소켓 정리
void GameServerManager::ErasePlayerAndSocket(Player* player,
    const std::string& args,
    int Sock) {
        {
            std::lock_guard<std::mutex> lock(heatbeatforplayer_MTX);
            heatbeatforplayer.erase(args);
        }
        {
            std::lock_guard<std::mutex> lock(players_MTX);
            players.erase(args);
        }
        if (Sock != -1) {
            std::lock_guard<std::mutex> lock(socketToHash_MTX);
            socketToHash.erase(Sock);
        }
        G_MemoryPool->release<Player>(player);
}

// 룸에서 플레이어 제거 및 정리
bool GameServerManager::RemovePlayerFromRoomAndCleanup(
    GameRoom* room, const std::string& args) {
    if (!room->removePlayerHash(args))
        return false;

    // 룸이 비었으면 메모리 반환
    if (room->getPlayerCount() == 0) {
        {
            std::lock_guard<std::mutex> lock(waitrooms_MTX);
            waitrooms.erase(room);
        }
        G_MemoryPool->release<GameRoom>(room);
    }
    return true;
}

// 플레이어 삭제
bool GameServerManager::DeletePlayer(const std::string& args) {
    Player* player_to_delete = nullptr;
    GameRoom* room = nullptr;
    int Sock = -1;

    {
        std::lock_guard<std::mutex> lock(players_MTX);
        auto find = players.find(args);
        if (find != players.end()) {
            player_to_delete = find->second;
        }
    }

    if (player_to_delete) {
        Sock = player_to_delete->Get_socket();
        if (Sock == -1)
            return false;

        if (player_to_delete->Getstatus() == playerstatus::INLOBBY) {
            ErasePlayerAndSocket(player_to_delete, args, Sock);
        }
        else if (player_to_delete->Getstatus() == playerstatus::INROOM) {
            room = player_to_delete->Get_Room();
            if (room) {
                RemovePlayerFromRoomAndCleanup(room, args);
            }
            ErasePlayerAndSocket(player_to_delete, args, Sock);
        }
        return true;
    }
    return false;
}

// 플레이어 조회
Player* GameServerManager::FindPlayer(const std::string& hash) {
    Player* player_find = nullptr;
    {
        std::lock_guard<std::mutex> lock(players_MTX);
        auto find = players.find(hash);
        if (find != players.end()) {
            player_find = find->second;
        }
    }
    return player_find;
}

// 룸 참가 또는 생성 (매칭 시스템)
bool GameServerManager::joinOrCreateRoom(const std::string& hash,
    const std::string& nickname) {
    GameRoom* Room = nullptr;
    std::unordered_set<std::string> playerhashes;
    Player* player = FindPlayer(hash);
    if (!player)
        return false;

    {
        std::lock_guard<std::mutex> lock(waitrooms_MTX);
        if (waitrooms.empty()) {
            // 대기 룸이 없으면 새로 생성
            Room = G_MemoryPool->acquire<GameRoom>();
            std::cout << '\n' << "[GameLobbyServer]" << "방이 없어서 방생성을 합니다  \n";
            waitrooms.insert(Room);
        }
        else {
            // 기존 대기 룸 사용
            Room = *waitrooms.begin();
        }
    }

    if (!Room)
        return false;
    if (!Room->addPlayerHash(hash))
        return false;

    player->SetplayerNickName(nickname);
    player->Register_Room(Room);

    if (Room->isFull()) {
        // 룸이 가득 차면 매칭 시작
        std::cout << '\n' << "[GameLobbyServer]" << "매칭 준비 완료 매칭을 시작합니다(룸에 할당된 데디 서버 룸에 속해있는 플레이어들에게 정보를 넘겨준다) \n";
        {
            std::lock_guard<std::mutex> lock(waitrooms_MTX);
            waitrooms.erase(Room);
        }
        {
            std::lock_guard<std::mutex> lock(startedrooms_MTX);
            auto [it, inserted] = startedrooms.insert(Room);
            GameRoom* Room = *it;
            playerhashes = Room->GetALLplayerHashes();
            SendGameStart(playerhashes);
        }
    }
    else {
        player->Setstatus(playerstatus::INROOM);
    }
    return true;
}

// 게임 시작 처리 (데디서버 할당 및 플레이어에게 정보 전송)
void GameServerManager::SendGameStart(
    const std::unordered_set<std::string>& in_playerhashes) {

    // 사용 가능한 데디케이티드 서버 조회
    auto [key, INfor] = G_DedicatedManger->GetDedicatedServerInfor();
    if (key == -1)return;

    std::vector<std::string> hashes(in_playerhashes.begin(),
        in_playerhashes.end());

    // 데디서버에 플레이어 인증 정보 전송
    G_Routine->SerializeAndSendResponseToDediPacket(key, false, hashes);

    // 각 플레이어에게 게임 서버 정보 전송
    Player* player = nullptr;
    std::lock_guard<std::mutex> lock(players_MTX);
    for (const auto& hash : in_playerhashes) {
        auto it = players.find(hash);
        if (it != players.end()) {
            player = it->second;
            player->Setstatus(playerstatus::INGAME);
            G_Routine->SerializeAndSendResponseForServerInfoPacket(
                player->Get_socket(), INfor->IP, INfor->Port, false);
            std::this_thread::sleep_for(std::chrono::milliseconds(800));
        }
    }
}

// 매칭 취소
bool GameServerManager::CancleRoom(const std::string& hash) {
    Player* player = FindPlayer(hash);
    GameRoom* Room = nullptr;
    if (!player)
        return false;

    {
        std::lock_guard<std::mutex> lock(waitrooms_MTX);
        auto it = waitrooms.find(player->Get_Room());
        if (it == waitrooms.end())
            return false;
        Room = *it;
    }

    if (!Room)
        return false;

    if (!RemovePlayerFromRoomAndCleanup(Room, hash))
        return false;

    player->Register_Room(nullptr);
    player->Setstatus(playerstatus::INLOBBY);
    return true;
}

// 안티치트 하트비트 응답 처리
void GameServerManager::ResponseHeartbeat(int sock, ResultType result) {
    std::string hash = GetsocketToHash(sock);
    if (hash.empty())
        return;

    {
        std::lock_guard<std::mutex> lock(heatbeatforplayer_MTX);
        auto thisplayer = heatbeatforplayer[hash];
    }

    std::cout << '\n' << "[GameLobbyServer]" << heatbeatforplayer[hash]->GetplayerNickName() << "에게 커널 안티치트 하트 비트 전송 받음\n";
    heatbeatforplayer[hash]->Set_AnticheatHeartbeat(
        std::chrono::steady_clock::now());
    heatbeatforplayer[hash]->Set_sendHeartbeat(false);
    heatbeatforplayer[hash]->Clear_Ban_stack();
}

// 안티치트 하트비트 체크 (주기적 실행)
void GameServerManager::Heartbeat(std::vector<uint8_t>* paket_data) {

    std::chrono::steady_clock::time_point now;
    std::chrono::seconds heartbeatTime;
    std::chrono::seconds heartbeatTimeout;
    std::vector<Player*> temp;
    temp.reserve(heatbeatforplayer.size());

    // 하트비트 체크 대상 수집
    {
        std::lock_guard<std::mutex> lock(heatbeatforplayer_MTX);
        for (auto& pair : heatbeatforplayer) {
            if (pair.second->Get_socket() == -1)
                continue;
            temp.push_back(pair.second);
        }
    }

    // 우선순위 큐로 오래된 하트비트 우선 처리
    std::priority_queue<Player*, std::vector<Player*>,
        PlayerHeartbeatComparator>
        tempQueue;

    for (Player* player : temp) {
        auto heartbeat = player->Get_AnticheatHeartbeat();
        now = std::chrono::steady_clock::now();
        heartbeatTimeout = std::chrono::seconds(400);
        heartbeatTime = std::chrono::seconds(60);

        if (now - heartbeat > heartbeatTime &&
            player->Getstatus() != playerstatus::INIT) {
            tempQueue.push(player);
        }
    }

    // 하트비트 전송 및 타임아웃 처리
    while (!tempQueue.empty()) {
        Player* player = tempQueue.top();
        tempQueue.pop();
        if (!player)
            continue;

        int PlayerSocket = player->Get_socket();
        if (PlayerSocket == -1)
            continue;

        auto heartbeat = player->Get_AnticheatHeartbeat();
        now = std::chrono::steady_clock::now();
        heartbeatTimeout = std::chrono::seconds(400);

        if (now - heartbeat > heartbeatTime && !player->IsSendHeartbeat()) {
            // 하트비트 전송
            std::cout << '\n' << "[GameLobbyServer]" << player->GetplayerNickName() << "에게 커널 안티치트 하트 비트 전송\n";
            G_Routine->SendPacketToClient(paket_data, PlayerSocket, false);
            player->Set_sendHeartbeat(true);
        }

        if (now - heartbeat > heartbeatTimeout)
        {
            // 타임아웃 시 연결 종료
            G_Routine->RequestDisconnect(PlayerSocket);
        }
    }
}