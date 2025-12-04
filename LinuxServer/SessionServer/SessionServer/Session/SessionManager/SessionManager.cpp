#include "SessionManager.h"
#include "../../MemoryPool/MemoryPool.h"
#include "../../Network/Packet/PacketStructure/PacketStructure.h"
#include "../ClientInfo/ClientInfo.h"
#include <mutex>
#include <iostream>

SessionManager::SessionManager() { clientCache.reserve(100000); }

SessionManager::~SessionManager() {}

// 인증 서버에서 로그인 시 세션 검증 (중복 로그인 체크)
ResultType
SessionManager::getPlayerStateForAuthserver(const std::string& hash) {

    ResultType Result;
    if (hash.empty())
        Result = ResultType::Login_Failed;
    else {
        {
            std::lock_guard<std::mutex> lock(Cache_MTX);
            auto client = clientCache.find(hash);

            if (client != clientCache.end()) {
                // 이미 로그인된 세션
                Result = ResultType::Login_AlreadyLoggedIn;
                std::cout << '\n' << "[세션서버]" << hash << "님은 로그인이 이미 되어있습니다 \n";
            }
            else {
                // 새로운 세션 생성
                clientCache[hash] = G_MemoryPool->acquire<ClientInfo>();
                std::cout << '\n' << "[세션서버]" << hash << "님은 로그인을 성공 했습니다 \n";
                Result = ResultType::Login_Succeeded;
            }
        }
    }
    return Result;
}

// 게임 로비 서버 접속 시 세션 검증 및 하드웨어 정보 등록
bool SessionManager::getPlayerStateForGameLobbyserver(const std::string& hash, const std::string& mainboard, const std::string& CPUID) {
    if (hash.empty())
        return false;

    std::lock_guard<std::mutex> lock(Cache_MTX);
    auto client = clientCache.find(hash);
    if (client != clientCache.end()) {
        // 하드웨어 정보 등록 (안티치트용)
        client->second->RegisterInfo(mainboard, CPUID);
        std::cout << '\n' << "[세션서버]" << hash << "님의 하드 정보 mainboard: " << mainboard << "cpu: " << CPUID << "가 서버에 등록 되었습니다" << '\n';
        return true;
    }
    return false;
}

// 로그아웃 처리 (세션 삭제)
void SessionManager::LogOut(const std::string& hash) {
    if (hash.empty())
        return;

    std::lock_guard<std::mutex> lock(Cache_MTX);
    auto client = clientCache.find(hash);
    if (client != clientCache.end()) {
        G_MemoryPool->release<ClientInfo>(client->second);
        clientCache.erase(client);
        std::cout << '\n' << "[세션서버]" << hash << "님 로그아웃 \n";
    }
    else {
        // 세션이 없는데 로그아웃 요청
    }
}