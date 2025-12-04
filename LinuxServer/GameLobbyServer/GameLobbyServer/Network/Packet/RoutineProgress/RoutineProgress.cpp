#include "RoutineProgress.h"
#include "../../../GameCore/GameServerManager/GameServerManager.h"
#include "../../../MemoryPool/MemoryPool.h"
#include "../../Aes/Aes.h"
#include "../../Network.h"
#include "../../SessionServerConnector/SessionServerConnector.h"
#include "../CompactBinaryReader/CompactBinaryReader.h"
#include "../OptimizedBinaryPacketSerializer/OptimizedBinaryPacketSerializer.h"
#include "../PacketStructure/PacketStructure.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <iostream>
#include "../../DedicatedManger/DedicatedManger.h"

std::unique_ptr<RoutineProgress> G_Routine =
std::make_unique<RoutineProgress>(3);

// Thread Pool 초기화 및 워커 스레드 생성
RoutineProgress::RoutineProgress(uint8_t in_threadcount)
    : threadcount(in_threadcount) {
    ProsessThreads_status.store(true);
    ProsessThreads.reserve(threadcount);
    for (int i = 0; i < threadcount; ++i) {
        ProsessThreads.emplace_back(&RoutineProgress::RoutineProgressWorkerThread,
            this, i);
    }
    // 안티치트 하트비트 스레드 시작
    HeatbeatThread = std::thread(&RoutineProgress::HeatbeatWoker, this,
        threadcount + 1);
}

RoutineProgress::~RoutineProgress() { Release(); }

// 연결 종료 요청
void RoutineProgress::RequestDisconnect(int clientsock) {
    G_network->Disconnect(clientsock);
}

// 네트워크 송신 큐에 데이터 추가
void RoutineProgress::SendData_to_Sendque(int clientsock,
    std::vector<uint8_t>* data,
    bool isLast) {
    G_network->addToSendQueue(clientsock, isLast, data);
}

// 암호화된 데이터 복사
std::vector<uint8_t>*
RoutineProgress::CopyEncryptedData(const std::vector<uint8_t>& source_data) {
    std::vector<uint8_t>* encrypted_data =
        G_MemoryPool->acquire<std::vector<uint8_t>>();
    encrypted_data->resize(source_data.size());
    *encrypted_data = source_data;
    return encrypted_data;
}

// 해시 패킷 직렬화 및 암호화
std::vector<uint8_t>* RoutineProgress::SerializeHashPacketResponse(
    const HashPacket& response_packet) {

    std::vector<uint8_t>* Serialize_data =
        G_MemoryPool->acquire<std::vector<uint8_t>>();
    std::vector<uint8_t>* encrypted_data =
        G_MemoryPool->acquire<std::vector<uint8_t>>();
    OptimizedBinaryPacketSerializer::SerializePacket<HashPacket>(response_packet,
        Serialize_data);
    std::unique_ptr<std::vector<uint8_t>,
        std::function<void(std::vector<uint8_t>*)>>
        readerguardfordata1(Serialize_data, [](std::vector<uint8_t>* p_data) {
        G_MemoryPool->release<std::vector<uint8_t>>(p_data);
            });

    AES->Aes_Encrypt(Serialize_data, encrypted_data);
    return encrypted_data;
}

// 데디케이티드 서버에 플레이어 인증 데이터 전송
void RoutineProgress::SerializeAndSendResponseToDediPacket(
    int Socket, bool isLast, const std::vector<std::string>& hashes) {
    std::vector<uint8_t>* Serialize_data =
        G_MemoryPool->acquire<std::vector<uint8_t>>();
    FUserAuthData VectorStringPacketpacket;
    VectorStringPacketpacket.Type = PacketType::GameCreate;
    VectorStringPacketpacket.hash = hashes;

    OptimizedBinaryPacketSerializer::SerializePacket<FUserAuthData>(
        VectorStringPacketpacket, Serialize_data);

    G_DedicatedManger->addToSendQueue(Socket, isLast, Serialize_data);
}

// 클라이언트에 게임 서버 정보 전송
void RoutineProgress::SerializeAndSendResponseForServerInfoPacket(
    int clientSocket, const std::string& IP, uint16_t port, bool isLast) {
    std::vector<uint8_t>* Serialize_data =
        G_MemoryPool->acquire<std::vector<uint8_t>>();
    ServerInfoPacket ServerInfoPacket_packet;
    ServerInfoPacket_packet.Type = PacketType::GameCreate;
    ServerInfoPacket_packet.IP = IP;
    ServerInfoPacket_packet.port = port;
    OptimizedBinaryPacketSerializer::SerializePacket<ServerInfoPacket>(
        ServerInfoPacket_packet, Serialize_data);
    std::vector<uint8_t>* encrypted_data =
        G_MemoryPool->acquire<std::vector<uint8_t>>();
    AES->Aes_Encrypt(Serialize_data, encrypted_data);
    SendData_to_Sendque(clientSocket, encrypted_data, isLast);
    G_MemoryPool->release<std::vector<uint8_t>>(Serialize_data);
}

// 결과 패킷 직렬화 및 전송
void RoutineProgress::SerializeAndSendResponseForResultPacket(
    std::vector<uint8_t>* data, const ResultPacket& response_packet,
    int clientSocket, bool isLast) {

    OptimizedBinaryPacketSerializer::SerializePacket<ResultPacket>(
        response_packet, data);
    std::vector<uint8_t>* encrypted_data =
        G_MemoryPool->acquire<std::vector<uint8_t>>();
    AES->Aes_Encrypt(data, encrypted_data);
    SendData_to_Sendque(clientSocket, encrypted_data, isLast);
}

// 범용 패킷 직렬화 (템플릿)
template <typename T>
void RoutineProgress::SerializePacket(std::vector<uint8_t>* data,
    const T& packet) {
    OptimizedBinaryPacketSerializer::SerializePacket<T>(packet, data);
}

// 패킷 암호화 및 클라이언트 전송
void RoutineProgress::SendPacketToClient(std::vector<uint8_t>* data,
    int clientSocket, bool isLast) {
    std::vector<uint8_t>* encrypted_data =
        G_MemoryPool->acquire<std::vector<uint8_t>>();
    AES->Aes_Encrypt(data, encrypted_data);
    SendData_to_Sendque(clientSocket, encrypted_data, isLast);
}

// 수신 패킷 복호화 및 타입별 처리
void RoutineProgress::HandleReceivedPacket(int clientSocket,
    const std::vector<uint8_t>& data) {
    try {
        if (!AES)
            return;

        size_t size = data.size();
        CompactBinaryReader* reader = nullptr;
        std::vector<uint8_t>* Serialize_data = nullptr;
        std::vector<uint8_t>* encrypted_data = nullptr;

        // 메모리 풀에서 버퍼 할당
        std::vector<uint8_t>* decrypted_data =
            G_MemoryPool->acquire<std::vector<uint8_t>>();

        // AES 복호화
        AES->Aes_Decrypt(&data, decrypted_data);
        reader = G_MemoryPool->acquire<CompactBinaryReader>();
        PacketType packet_type;

        // RAII 패턴으로 메모리 자동 반환
        std::unique_ptr<CompactBinaryReader,
            std::function<void(CompactBinaryReader*)>>
            readerguardforCompactBinaryReader(
                reader, [](CompactBinaryReader* p_reader) {
                    G_MemoryPool->release<CompactBinaryReader>(p_reader);
                });
        std::unique_ptr<std::vector<uint8_t>,
            std::function<void(std::vector<uint8_t>*)>>
            readerguardfordata1(Serialize_data, [](std::vector<uint8_t>* p_data) {
            G_MemoryPool->release<std::vector<uint8_t>>(p_data);
                });
        std::unique_ptr<std::vector<uint8_t>,
            std::function<void(std::vector<uint8_t>*)>>
            readerguardfordata2(decrypted_data, [](std::vector<uint8_t>* p_data) {
            G_MemoryPool->release<std::vector<uint8_t>>(p_data);
                });

        if (!OptimizedBinaryPacketSerializer::ParseSecurePacket(
            *decrypted_data, packet_type, reader) ||
            !reader) {
            return;
        }

        // 패킷 타입별 처리
        switch (packet_type) {
        case PacketType::TryConnectLobbyServerRequest: {
            // 로비 서버 접속 요청 (세션 검증)
            IntegrityCheckPacket packet;
            OptimizedBinaryPacketSerializer::DeserializePacket<IntegrityCheckPacket>(
                *reader, packet);
            if (packet.hash.empty())
                return;
            {
                std::lock_guard<std::mutex> lock(client_sockets_mutex);
                client_sockets[packet.hash] = clientSocket;
            }
            std::cout << '\n' << "[AuthRoutineServer]" << packet.hash << "님 로비 서버 접근 체크  \n";
            encrypted_data = CopyEncryptedData(data);
            G_SessionServerConnector->addToSessionServerQueue(encrypted_data);
            break;
        }
        case PacketType::TryConnectLobbyServerResponse: {
            // 세션 서버로부터 검증 응답
            ResultAndHashPacket packet;
            OptimizedBinaryPacketSerializer::DeserializePacket<ResultAndHashPacket>(
                *reader, packet);
            if (packet.hash.empty())
                return;
            int thissocket = Find_socket(packet.hash);
            if (thissocket == -1)
                return;

            encrypted_data = CopyEncryptedData(data);
            SendData_to_Sendque(thissocket, encrypted_data, false);
            if (packet.ResultTypes != ResultType::CheckSession_Succeeded) {
                {
                    std::lock_guard<std::mutex> lock(client_sockets_mutex);
                    client_sockets.erase(packet.hash);
                }
                return;
            }
            else {
                std::cout << '\n' << "[AuthRoutineServer]" << packet.hash << "님 로비 서버 접근 체크 성공!!! \n";
                G_gameservermanager->HandleLoginAttempt(thissocket, packet.hash);
            }
            break;
        }
        case PacketType::LogoutRequest: {
            // 로그아웃 요청 처리
            HashPacket packet;
            ResultPacket Responsepacket;
            OptimizedBinaryPacketSerializer::DeserializePacket<HashPacket>(*reader,
                packet);
            if (packet.hash.empty())
                return;

            encrypted_data = CopyEncryptedData(data);
            G_SessionServerConnector->addToSessionServerQueue(encrypted_data);

            Responsepacket.Type = PacketType::LogoutResponse;
            if (!G_gameservermanager->DeletePlayer(packet.hash)) {
                Responsepacket.ResultTypes = ResultType::LogOut_Failed;
            }
            else {
                Responsepacket.ResultTypes = ResultType::LogOut_Succeeded;
            }
            Serialize_data = G_MemoryPool->acquire<std::vector<uint8_t>>();
            SerializeAndSendResponseForResultPacket(Serialize_data, Responsepacket,
                clientSocket, true);
            break;
        }
        case PacketType::JoinRoomRequest: {
            // 매칭 룸 참가 요청
            TwoStringPacket packet;
            ResultPacket Responsepacket;
            OptimizedBinaryPacketSerializer::DeserializePacket<TwoStringPacket>(
                *reader, packet);
            if (packet.str1.empty())
                return;
            if (packet.str2.empty())
                return;
            Responsepacket.Type = PacketType::JoinRoomResponse;
            std::cout << '\n' << "[GameLobbyServer]" << packet.str2 << " 님이 매칭 대기중  \n";
            if (!G_gameservermanager->joinOrCreateRoom(packet.str1, packet.str2)) {
                Responsepacket.ResultTypes = ResultType::JoinRoom_Failed;
            }
            else {
                Responsepacket.ResultTypes = ResultType::JoinRoom_Succeeded;
            }
            Serialize_data = G_MemoryPool->acquire<std::vector<uint8_t>>();
            SerializeAndSendResponseForResultPacket(Serialize_data, Responsepacket,
                clientSocket, false);
            break;
        }
        case PacketType::CancelRoomRequest: {
            // 매칭 취소 요청
            HashPacket packet;
            ResultPacket Responsepacket;
            OptimizedBinaryPacketSerializer::DeserializePacket<HashPacket>(*reader,
                packet);
            if (packet.hash.empty())
                return;
            Responsepacket.Type = PacketType::CancelRoomResponse;
            if (!G_gameservermanager->CancleRoom(packet.hash)) {
                Responsepacket.ResultTypes = ResultType::CancelRoom_Failed;
            }
            else {
                Responsepacket.ResultTypes = ResultType::CancelRoom_Succeeded;
            }
            Serialize_data = G_MemoryPool->acquire<std::vector<uint8_t>>();
            SerializeAndSendResponseForResultPacket(Serialize_data, Responsepacket,
                clientSocket, false);
            break;
        }
        case PacketType::ANTI_EVENT_REQUEST: {
            // 안티치트 이벤트 수집 (프로세스 목록 등)
            stringforVectorPacket packet;
            OptimizedBinaryPacketSerializer::DeserializePacket<stringforVectorPacket>(*reader,
                packet);
            std::cout << '\n' << "[GameLobbyServer]" << packet.hash << " 님 행위 체크(필터x)  :";
            for (const auto& it : packet.str)
            {
                std::cout << "프로그램명 :" << it;
                std::cout << '\n';
                // 추후 행위기반 코드
            }
        }
        case PacketType::HeartbeatResponse: {
            // 안티치트 하트비트 응답
            ResultPacket packet;
            OptimizedBinaryPacketSerializer::DeserializePacket<ResultPacket>(*reader,
                packet);
            G_gameservermanager->ResponseHeartbeat(clientSocket, packet);
        }
        default:
            break;
        }
    }
    catch (...) {
        return;
    }
}

// 소켓 연결 종료 시 정리
bool RoutineProgress::CleanupforSocket(int socket) {
    std::string hash = G_gameservermanager->GetsocketToHash(socket);
    if (hash.empty())
        return false;
    if (!G_gameservermanager->DeletePlayer(hash))
        return false;
    Logoutpacket(hash);
    return true;
}

// 세션 토큰으로 소켓 조회
int RoutineProgress::Find_socket(const std::string& hash) {
    std::lock_guard<std::mutex> lock(client_sockets_mutex);
    auto it = client_sockets.find(hash);
    if (it != client_sockets.end()) {
        return it->second;
    }
    return -1;
}

// 패킷을 처리 큐에 추가 (Producer)
void RoutineProgress::addToProgressQueue(ClientPacket* Q_data) {
    {
        std::lock_guard<std::mutex> lock(routine_queue_mutex);
        data_queue.push(Q_data);
    }
    wakeUpthread.notify_one();
}

// 리소스 정리 및 스레드 종료
void RoutineProgress::Release() {
    ProsessThreads_status.store(false);
    wakeUpthread.notify_all();

    if (HeatbeatThread.joinable())
        HeatbeatThread.join();

    for (auto& thread : ProsessThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    // 큐에 남은 패킷 메모리 반환
    {
        std::lock_guard<std::mutex> lock(routine_queue_mutex);
        while (!data_queue.empty()) {
            ClientPacket* data = data_queue.front();
            data_queue.pop();
            G_MemoryPool->release<ClientPacket>(data);
        }
    }
}

// 워커 스레드 실행 함수 (Consumer)
void RoutineProgress::RoutineProgressWorkerThread(int threadId) {
    ClientPacket* data = nullptr;
    while (ProsessThreads_status.load()) {
        data = nullptr;
        {
            std::unique_lock<std::mutex> lock(routine_queue_mutex);
            wakeUpthread.wait(lock, [this]() {
                return !data_queue.empty() || !ProsessThreads_status.load();
                });

            if (!ProsessThreads_status.load())
                break;
            data = data_queue.front();
            data_queue.pop();
        }

        // RAII 패턴으로 메모리 자동 반환
        std::unique_ptr<ClientPacket, std::function<void(ClientPacket*)>> reguard(
            data,
            [](ClientPacket* data) { G_MemoryPool->release<ClientPacket>(data); });

        // 패킷 처리
        HandleReceivedPacket(data->clientSocket, data->data);
    }
}

// 로그아웃 패킷 세션 서버로 전송
void RoutineProgress::Logoutpacket(const std::string& hash) {
    HashPacket packet;
    packet.Type = PacketType::LogoutRequest;
    packet.hash = hash;
    std::vector<uint8_t>* encrypted_data = SerializeHashPacketResponse(packet);
    if (encrypted_data->size() == 0)
        return;

    G_SessionServerConnector->addToSessionServerQueue(encrypted_data);
}

// 안티치트 하트비트 워커 스레드
void RoutineProgress::HeatbeatWoker(int threadId) {
    std::vector<uint8_t> paket_data;
    TypePacket HeatBeatpacket;
    HeatBeatpacket.Type = PacketType::HeartbeatRequest;
    G_Routine->SerializePacket(&paket_data, HeatBeatpacket);

    while (ProsessThreads_status.load()) {
        G_gameservermanager->Heartbeat(&paket_data);
    }
}