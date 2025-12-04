#include "SessionServerConnector.h"
#include "../../MemoryPool/MemoryPool.h"
#include "../Packet/RoutineProgress/RoutineProgress.h"
#include <arpa/inet.h>
#include <functional>
#include <mutex>
#include <netinet/in.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <iostream>

std::unique_ptr<SessionServerConnector> G_SessionServerConnector =
std::make_unique<SessionServerConnector>();

SessionServerConnector::SessionServerConnector() {}

SessionServerConnector::~SessionServerConnector() { Release(); }

// 세션 서버 연결
bool SessionServerConnector::ConnectSession() {
    SessionSocket = socket(AF_INET, SOCK_STREAM, 0);

    // 수신 타임아웃 설정
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    // 세션 서버 주소 설정
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8100);
    addr.sin_addr.s_addr = inet_addr("172.30.1.40");

    if (setsockopt(SessionSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout,
        sizeof(timeout)) < 0) {
        close(SessionSocket);
        return false;
    }

    // 세션 서버 연결
    if (connect(SessionSocket, (sockaddr*)&addr, sizeof(addr)) < 0) {
        switch (errno) {
        case ECONNREFUSED:
            std::cout << "서버가 연결을 거부했습니다 (ECONNREFUSED)" << std::endl;
            break;
        case ETIMEDOUT:
            std::cout << "연결 시간이 초과되었습니다 (ETIMEDOUT)" << std::endl;
            break;
        case ENETUNREACH:
            std::cout << "네트워크에 도달할 수 없습니다 (ENETUNREACH)" << std::endl;
            break;
        case EADDRINUSE:
            std::cout << "주소가 이미 사용 중입니다 (EADDRINUSE)" << std::endl;
            break;
        case EINPROGRESS:
            std::cout << "논블로킹 소켓에서 연결 진행 중입니다 (EINPROGRESS)" << std::endl;
            break;
        default:
            break;
        }
        close(SessionSocket);
    }

    // 송수신 워커 스레드 시작
    ConnectSessionserver_running.store(true);
    sendwokerthread = std::thread(&SessionServerConnector::SendWoker, this);
    Recvwokerthread = std::thread(&SessionServerConnector::RecvWorker, this);

    return true;
}

// 리소스 정리
void SessionServerConnector::Release() {
    if (!ConnectSessionserver_running.load())
        return;
    ConnectSessionserver_running.store(false);

    wakeUpSendthread.notify_all();

    if (SessionSocket != -1) {
        shutdown(SessionSocket, SHUT_RDWR);
        close(SessionSocket);
        SessionSocket = -1;
    }

    if (sendwokerthread.joinable()) {
        sendwokerthread.join();
    }

    if (Recvwokerthread.joinable()) {
        Recvwokerthread.join();
    }

    // 송신 큐 정리
    {
        std::lock_guard<std::mutex> lock(sendToSessionServer_queue_Mtx);
        while (!sendToSessionServer_queue.empty()) {
            std::vector<u_int8_t>* data = sendToSessionServer_queue.front();
            sendToSessionServer_queue.pop();
            G_MemoryPool->release(data);
        }
    }
}

// 세션 서버 송신 큐에 데이터 추가
void SessionServerConnector::addToSessionServerQueue(
    std::vector<u_int8_t>* data) {
        {
            std::lock_guard<std::mutex> sendToSessionServer_queue_lock(
                sendToSessionServer_queue_Mtx);
            sendToSessionServer_queue.push(data);
        }
        wakeUpSendthread.notify_one();
}

// 송신 워커 스레드
void SessionServerConnector::SendWoker() {
    std::vector<u_int8_t>* data = nullptr;

    while (ConnectSessionserver_running.load()) {
        {
            std::unique_lock<std::mutex> lock(sendToSessionServer_queue_Mtx);
            wakeUpSendthread.wait(lock, [&]() {
                return !ConnectSessionserver_running.load() ||
                    !sendToSessionServer_queue.empty();
                });

            data = sendToSessionServer_queue.front();
            sendToSessionServer_queue.pop();
        }

        // RAII 패턴으로 메모리 자동 반환
        std::unique_ptr<std::vector<u_int8_t>,
            std::function<void(std::vector<u_int8_t>*)>>
            reguard(data,
                [](std::vector<u_int8_t>* ptr) { G_MemoryPool->release(ptr); });

        // 세션 서버로 전송
        send(SessionSocket, data->data(), data->size(), 0);
    }
}

// 수신 워커 스레드
void SessionServerConnector::RecvWorker() {
    ClientPacket* Cpacket;
    while (ConnectSessionserver_running.load()) {

        Cpacket = G_MemoryPool->acquire<ClientPacket>();
        Cpacket->data.resize(1024);
        Cpacket->clientSocket = -1;

        int bytes = recv(SessionSocket, Cpacket->data.data(), 1024, 0);

        if (bytes > 0) {
            Cpacket->data.resize(bytes);
            G_Routine->addToProgressQueue(Cpacket);
        }
        else {
            if (errno == ECONNRESET || errno == ENOTCONN) {
                // 연결 끊김 에러 처리
            }
        }
    }
}