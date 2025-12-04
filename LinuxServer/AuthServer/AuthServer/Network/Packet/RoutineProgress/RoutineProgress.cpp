
취업용 으로 내가 만든 소스코드 Github 에 올릴거거든 내소스코드에다가 주석으로 써줄수있어 ? ? 일단 1번째 소스코드 나 변수명 절대 수정하지마

네, 소스코드에 주석을 달아드리겠습니다.변수명은 절대 수정하지 않고 주석만 추가하겠습니다.

1번째 소스코드를 보여주시면 취업용 포트폴리오에 적합하게 상세한 주석을 달아드리겠습니다.코드를 올려주세요!


RoutineProgress.cpp
269줄

cpp


RoutineProgress.h
62줄

h

취업용 포트폴리오에 적합하게 상세한 주석을 추가했습니다.변수명은 전혀 수정하지 않았습니다.

Failed to create
RoutineProgress
취업용 포트폴리오를 위해 RoutineProgress.h 헤더 파일에 상세한 주석 추가(출력용)
다 지우고 상세한 주석은 필없어


기존 파일 삭제


다시 원하는거 말해줄게 취업용 소스코드이고 소스코드 나 변수 절대 수정하지마 이거 github에 올릴건데 github 취업용 주석 써주고 상세한 주석을 필요없어

네, 알겠습니다!

취업용 GitHub 포트폴리오에 적합하게 간결하고 핵심적인 주석만 달아드리겠습니다.변수명과 코드는 절대 수정하지 않겠습니다.

소스코드를 보여주세요!


RoutineProgress.cpp
269줄

cpp


RoutineProgress.h
62줄

h

네, 취업용 GitHub에 적합하게 간결한 주석만 달아드리겠습니다!


취업용 GitHub 포트폴리오를 위한 RoutineProgress.h(간결한 주석)
RoutineProgress.h

취업용 GitHub 포트폴리오를 위한 RoutineProgress.cpp(간결한 주석)
RoutineProgress.cpp
완성했습니다!

취업용 GitHub에 적합하게 간결하고 핵심적인 주석만 달았습니다 :

각 함수의 역할을 한 줄로 설명
중요 로직에만 주석 추가
변수명과 코드는 절대 수정하지 않음
RoutineProgress.h 다운로드 RoutineProgress.cpp 다운로드

Routineprogress
H
Routineprogress
CPP







Routineprogress · CPP
복사

#include "RoutineProgress.h"
#include "../../../MemoryPool/MemoryPool.h"
#include "../../Aes/Aes.h"
#include "../../ServerConnector/SessionServerConnector/SessionServerConnector.h"
#include "../../Mafia_DB/MafiaDatabase.h"
#include "../../Network.h"
#include "../CompactBinaryReader/CompactBinaryReader.h"
#include "../OptimizedBinaryPacketSerializer/OptimizedBinaryPacketSerializer.h"
#include "../PacketStructure/PacketStructure.h"
#include <cstdint>
#include <exception>
#include <functional>
#include <iostream>

// Thread Pool 초기화 및 워커 스레드 생성
RoutineProgress::RoutineProgress(uint8_t in_threadcount)
    : threadcount(in_threadcount), ProsessThreads_status(true) {

    MafiaDB_server = std::make_unique<MafiaDatabase>();

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
    G_network->addToSendQueue(clientsock, false, data);
}

// 처리 결과 패킷을 클라이언트에 전송
void RoutineProgress::SendResultPacket(int clientsock, PacketType type,
    ResultType result,
    const std::string& hash) {
    try {
        // 메모리 풀에서 버퍼 할당
        std::vector<uint8_t>* serialized_data =
            G_MemoryPool->acquire<std::vector<uint8_t>>();
        std::unique_ptr<std::vector<uint8_t>,
            std::function<void(std::vector<uint8_t>*)>>
            reguard(serialized_data, [](std::vector<uint8_t>* ptr) {
            G_MemoryPool->release<std::vector<uint8_t>>(ptr);
                });

        // 패킷 타입에 따라 직렬화
        if (type == PacketType::RegisterResponse) {
            ResultPacket data{ type, result };
            OptimizedBinaryPacketSerializer::SerializePacket<
                ResultPacket>(data, serialized_data);
        }
        else if (type == PacketType::LoginResponse) {

            ResultAndHashPacket data{ type, result, hash };
            OptimizedBinaryPacketSerializer::SerializePacket<ResultAndHashPacket>(
                data, serialized_data);
        }
        else {
            return;
        }

        // AES 암호화 후 전송
        std::vector<uint8_t>* To_ClientPacket =
            G_MemoryPool->acquire<std::vector<uint8_t>>();
        AES->Aes_Encrypt(serialized_data, To_ClientPacket);
        SendData_to_Sendque(clientsock, To_ClientPacket);

    }
    catch (const std::exception&) {
        return;
    }
}

// 수신 패킷 복호화 및 타입별 처리
void RoutineProgress::HandleReceivedPacket(int clientSocket,
    const std::vector<uint8_t>& data) {
    try {
        if (!AES)
            return;

        CompactBinaryReader* reader = nullptr;

        // 메모리 풀에서 버퍼 할당
        std::vector<uint8_t>* decrypted_data =
            G_MemoryPool->acquire<std::vector<uint8_t>>();

        // AES 복호화
        AES->Aes_Decrypt(&data, decrypted_data);

        // 패킷 리더 할당
        reader = G_MemoryPool->acquire<CompactBinaryReader>();
        PacketType packet_type;

        // 복호화된 데이터 파싱
        if (!OptimizedBinaryPacketSerializer::ParseSecurePacket(
            *decrypted_data, packet_type, reader) ||
            !reader) {
            return;
        }

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

        // 패킷 타입별 처리
        switch (packet_type)
        {
        case PacketType::RegisterRequest:
            HandleRegisterPacket(clientSocket, *reader);
            break;
        case PacketType::LoginRequest:
            HandleLoginPacketWrapper(clientSocket, *reader);
            break;
        case PacketType::LogoutRequest:
            HandleLogoutPacketWrapper(clientSocket, *reader);
            break;
        default:
            break;
        }
    }
    catch (...) {
        return;
    }
}

// 회원가입 요청 처리
void RoutineProgress::HandleRegisterPacket(int clientSocket,
    CompactBinaryReader& reader) {
    if (!MafiaDB_server) {
        SendResultPacket(clientSocket, PacketType::RegisterResponse,
            ResultType::SignUp_Failed);
        return;
    }

    TwoStringPacket registerData;
    OptimizedBinaryPacketSerializer::DeserializePacket<TwoStringPacket>(
        reader, registerData);

    if (registerData.str1.empty() || registerData.str2.empty()) {
        SendResultPacket(clientSocket, PacketType::RegisterResponse,
            ResultType::SignUp_Failed);
        return;
    }

    // DB에 계정 생성
    ResultType result = MafiaDB_server->Sign_up(registerData.str1, registerData.str2);

    std::cout << '\n' << "[AuthServer]" << registerData.str1 << "님은 회원가입에 " << (result == ResultType::SignUp_Succeeded ? "성공하였습니다." : "실패하였습니다.") << '\n';
    SendResultPacket(clientSocket, PacketType::RegisterResponse, result);
}

// 로그인 요청 처리
void RoutineProgress::HandleLoginPacketWrapper(int clientSocket,
    CompactBinaryReader& reader) {
    if (!MafiaDB_server) {
        SendResultPacket(clientSocket, PacketType::LoginResponse,
            ResultType::Login_Failed);
        return;
    }

    TwoStringPacket loginData;
    OptimizedBinaryPacketSerializer::DeserializePacket<TwoStringPacket>(
        reader, loginData);

    if (loginData.str1.empty() || loginData.str2.empty()) {
        SendResultPacket(clientSocket, PacketType::LoginResponse,
            ResultType::Login_Failed);
        return;
    }

    std::string hash;
    hash.reserve(64);
    ResultType result =
        HandleLoginPacket(clientSocket, loginData.str1, loginData.str2, hash);

    // 로그인 성공 시 세션 서버에 토큰 전달
    if (result == ResultType::Login_Succeeded) {
        std::vector<uint8_t>* serialized_data =
            G_MemoryPool->acquire<std::vector<uint8_t>>();
        std::vector<uint8_t>* Encrypt_data =
            G_MemoryPool->acquire<std::vector<uint8_t>>();
        HashPacket data{ PacketType::LoginRequest, hash };
        OptimizedBinaryPacketSerializer::SerializePacket<HashPacket>(
            data, serialized_data);
        AES->Aes_Encrypt(serialized_data, Encrypt_data);

        G_MemoryPool->release<std::vector<uint8_t>>(serialized_data);
        return G_SessionServerConnector->addToSessionServerQueue(clientSocket, hash,
            Encrypt_data);
    }

    hash.clear();
    return SendResultPacket(clientSocket, PacketType::LoginResponse, result, hash);
}

// 로그아웃 요청 처리
void RoutineProgress::HandleLogoutPacketWrapper(int clientSocket,
    CompactBinaryReader& reader) {
    HashPacket logoutData;
    OptimizedBinaryPacketSerializer::DeserializePacket<HashPacket>(
        reader, logoutData);

    if (!logoutData.hash.empty()) {
        HandleLogOutPacket(clientSocket, logoutData.hash);
    }
}

// DB 로그인 검증 및 세션 토큰 생성
ResultType RoutineProgress::HandleLoginPacket(int clientSocket,
    const std::string& id,
    const std::string& pw,
    std::string& hash) {
    if (!MafiaDB_server || id.empty() || pw.empty()) {
        return ResultType::Login_Failed;
    }

    return MafiaDB_server->Sign_in(id, pw, hash);
}

// 로그아웃 처리
bool RoutineProgress::HandleLogOutPacket(int clientSocket,
    const std::string& hash) {
    return !hash.empty();
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

    if (MafiaDB_server) {
        MafiaDB_server->Release();
        MafiaDB_server.reset();
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
