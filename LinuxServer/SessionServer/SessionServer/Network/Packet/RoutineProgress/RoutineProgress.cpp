#include "RoutineProgress.h"
#include "../../../MemoryPool/MemoryPool.h"
#include "../../Aes/Aes.h"
#include "../../Network.h"
#include "../../../Session/SessionManager/SessionManager.h"
#include "../CompactBinaryReader/CompactBinaryReader.h"
#include "../OptimizedBinaryPacketSerializer/OptimizedBinaryPacketSerializer.h"
#include "../PacketStructure/PacketStructure.h"
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>

// Thread Pool 초기화 및 워커 스레드 생성
RoutineProgress::RoutineProgress(uint8_t in_threadcount)
    : threadcount(in_threadcount), ProsessThreads_status(true) {
    P_SessionManager = std::make_unique<SessionManager>();
    ProsessThreads.reserve(threadcount);
    for (int i = 0; i < threadcount; ++i) {
        ProsessThreads.emplace_back(&RoutineProgress::RoutineProgressWorkerThread,
            this, i);
    }
}

RoutineProgress::~RoutineProgress() { Release(); }

// 네트워크 송신 큐에 데이터 추가
void RoutineProgress::SendData_to_Sendque(int clientsock,
    std::vector<uint8_t>* data) {
    G_network->addToSendQueue(clientsock, data);
}

// 수신 패킷 복호화 및 타입별 처리
void RoutineProgress::HandleReceivedPacket(int clientSocket,
    const std::vector<uint8_t>& data) {
    try {
        if (!AES)
            return;

        size_t size = data.size();
        CompactBinaryReader* reader = nullptr;
        std::vector<uint8_t>* Serialize_data;
        std::vector<uint8_t>* encrypted_data;

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
            readerguardfordata(decrypted_data, [](std::vector<uint8_t>* p_data) {
            G_MemoryPool->release<std::vector<uint8_t>>(p_data);
                });

        if (!OptimizedBinaryPacketSerializer::ParseSecurePacket(
            *decrypted_data, packet_type, reader) ||
            !reader) {
            return;
        }

        // 패킷 타입별 처리
        switch (packet_type) {
        case PacketType::LoginRequest: {
            // 인증 서버로부터 로그인 요청 (세션 생성 및 중복 로그인 체크)
            HashPacket hashpacket;
            ResultAndHashPacket packet;
            OptimizedBinaryPacketSerializer::DeserializePacket<HashPacket>(*reader, hashpacket);
            packet.Type = PacketType::LoginResponse;
            packet.ResultTypes = P_SessionManager->getPlayerStateForAuthserver(hashpacket.hash);
            packet.hash = hashpacket.hash;

            Serialize_data = G_MemoryPool->acquire<std::vector<uint8_t>>();
            encrypted_data = G_MemoryPool->acquire<std::vector<uint8_t>>();
            OptimizedBinaryPacketSerializer::SerializePacket<ResultAndHashPacket>(packet, Serialize_data);

            AES->Aes_Encrypt(Serialize_data, encrypted_data);
            G_MemoryPool->release<std::vector<uint8_t>>(Serialize_data);
            SendData_to_Sendque(clientSocket, encrypted_data);
            break;
        }
        case PacketType::TryConnectLobbyServerRequest: {
            // 게임 로비 서버로부터 세션 검증 요청 (하드웨어 정보 등록)
            IntegrityCheckPacket hashpacket;
            ResultAndHashPacket packet;
            OptimizedBinaryPacketSerializer::DeserializePacket<IntegrityCheckPacket>(*reader, hashpacket);
            bool result = P_SessionManager->getPlayerStateForGameLobbyserver(hashpacket.hash, hashpacket.Mainboard_ID, hashpacket.CPU_ID);
            packet.Type = PacketType::TryConnectLobbyServerResponse;
            packet.ResultTypes = result ? ResultType::CheckSession_Succeeded : ResultType::CheckSession_Failed;
            packet.hash = hashpacket.hash;

            Serialize_data = G_MemoryPool->acquire<std::vector<uint8_t>>();
            encrypted_data = G_MemoryPool->acquire<std::vector<uint8_t>>();
            OptimizedBinaryPacketSerializer::SerializePacket<ResultAndHashPacket>(packet, Serialize_data);

            AES->Aes_Encrypt(Serialize_data, encrypted_data);
            G_MemoryPool->release<std::vector<uint8_t>>(Serialize_data);
            SendData_to_Sendque(clientSocket, encrypted_data);
        }
        case PacketType::LogoutRequest:
        {
            // 로그아웃 요청 (세션 삭제)
            HashPacket hashpacket;
            ResultAndHashPacket packet;
            OptimizedBinaryPacketSerializer::DeserializePacket<HashPacket>(*reader, hashpacket);
            P_SessionManager->LogOut(hashpacket.hash);
        }
        default:
            break;
        }
    }
    catch (...) {
        return;
    }
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

    // 모든 워커 스레드 종료 대기
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

            // 큐에 패킷이 들어올 때까지 대기
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