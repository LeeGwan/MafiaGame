#include "Network.h"
#include "../MemoryPool/MemoryPool.h"
#include "Packet/RoutineProgress/RoutineProgress.h"
#include "SessionServerConnector/SessionServerConnector.h"
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <queue>
#include <sys/types.h>
#include <unistd.h>

std::unique_ptr<Network> G_network = std::make_unique<Network>();

Network::Network() : port(8020), server_fd(-1), epoll_fd(-1) {}

// 네트워크 초기화 및 서버 소켓 생성
bool Network::Init() {
    struct epoll_event event;

    // 서버 소켓 생성
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        return false;
    }

    // SO_REUSEADDR 옵션 설정
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(server_fd);
        server_fd = -1;
        return false;
    }

    // 서버 주소 설정
    int serveraddrsize = sizeof(server_addr);
    std::memset(&server_addr, 0x0, serveraddrsize);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // 세션 서버 연결
    if (!G_SessionServerConnector->ConnectSession())return false;

  
    if (bind(server_fd, (struct sockaddr*)&server_addr, serveraddrsize)) {
        close(server_fd);
        server_fd = -1;
        return false;
    }

   
    if (listen(server_fd, SOMAXCONN)) {
        close(server_fd);
        server_fd = -1;
        return false;
    }

    // 논블로킹 설정
    if (!set_nonblocking(server_fd)) {
        close(server_fd);
        server_fd = -1;
        return false;
    }

    // Epoll 생성 및 서버 소켓 등록
    epoll_fd = epoll_create(1);
    if (epoll_fd == -1) {
        close(server_fd);
        server_fd = -1;
        return false;
    }

    event.events = EPOLLIN;
    event.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, event.data.fd, &event) == -1) {
        close(server_fd);
        close(epoll_fd);
        server_fd = -1;
        epoll_fd = -1;
        return false;
    }

    return true;
}

Network::~Network() { Release(); }

// 네트워크 시작 및 워커 스레드 생성
void Network::StartNetwork(unsigned int recvthreadcount,
    unsigned int sendthreadcount) {

    running.store(true);
    recv_epoll_fd.resize(recvthreadcount);
    send_epoll_fd.resize(sendthreadcount);
    send_queues.reserve(0x10000);

    // 수신 워커 스레드 생성
    for (unsigned int i = 0; i < recvthreadcount; ++i) {
        recv_epoll_fd[i] = epoll_create(1);
        if (recv_epoll_fd[i] == -1) {
            Release();
            return;
        }
        recvThreads.emplace_back(&Network::recvWorkerThread, this, i);
    }

    // 송신 워커 스레드 생성
    for (unsigned int i = 0; i < sendthreadcount; ++i) {
        send_epoll_fd[i] = epoll_create(1);
        if (send_epoll_fd[i] == -1) {
            Release();
            return;
        }
        sendThreads.emplace_back(&Network::sendWorkerThread, this, i);
    }

    AcceptConnection(recvthreadcount);
    return;
}

// 리소스 정리 및 모든 스레드 종료
void Network::Release() {

    if (!running.load())return;

    running.store(false);

    // 수신 스레드 종료
    for (auto& thread : recvThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    recvThreads.clear();

    // 송신 스레드 종료
    for (auto& thread : sendThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    sendThreads.clear();

    // 송신 큐 정리
    {
        std::lock_guard<std::mutex> lock(send_queue_mutex);
        for (auto& pair : send_queues) {
            while (!pair.second.empty()) {
                auto data = pair.second.front();
                pair.second.pop();
                G_MemoryPool->release<std::vector<uint8_t>>(data.second);
            }
        }
        send_queues.clear();
    }

    if (server_fd != -1) {
        close(server_fd);
        server_fd = -1;
    }

    if (epoll_fd != -1) {
        close(epoll_fd);
        epoll_fd = -1;
    }

    for (int recv_fd : recv_epoll_fd) {
        if (recv_fd != -1) {
            close(recv_fd);
        }
    }
    recv_epoll_fd.clear();

    for (int send_fd : send_epoll_fd) {
        if (send_fd != -1) {
            close(send_fd);
        }
    }
    send_epoll_fd.clear();
}

// 논블로킹 소켓 설정
bool Network::set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

// 수신 워커 스레드 (Epoll 기반)
void Network::recvWorkerThread(int threadId) {
    struct epoll_event events[MAX_EVENTS];
    ClientPacket* Cpacket = nullptr;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    while (running.load()) {
        int eventsize =
            epoll_wait(recv_epoll_fd[threadId], events, MAX_EVENTS, 10);
        if (eventsize == -1) {
            continue;
        }

        for (int i = 0; i < eventsize; ++i) {
            int fd = events[i].data.fd;
            if (events[i].events & EPOLLIN) {
                // 메모리 풀에서 패킷 할당
                Cpacket = G_MemoryPool->acquire<ClientPacket>();
                Cpacket->data.resize(BUFFER_SIZE);
                Cpacket->clientSocket = fd;

                ssize_t bytes_received = recv(fd, Cpacket->data.data(), BUFFER_SIZE, 0);
                if (bytes_received > 0) {
                    try {
                        Cpacket->data.resize(bytes_received);
                        G_Routine->addToProgressQueue(Cpacket);
                    }
                    catch (const std::exception& e) {
                        G_MemoryPool->release<ClientPacket>(Cpacket);
                        removeConnection(fd);
                    }
                }
                else if (bytes_received == 0) {
                    // 연결 종료
                    G_MemoryPool->release<ClientPacket>(Cpacket);
                    removeConnection(fd);
                }
                else {
                    G_MemoryPool->release<ClientPacket>(Cpacket);
                    if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        removeConnection(fd);
                    }
                }
            }
        }
    }
}

// 송신 워커 스레드 (Epoll 기반)
void Network::sendWorkerThread(int threadId) {
    struct epoll_event events[MAX_EVENTS];
    while (running.load()) {
        int eventsize =
            epoll_wait(send_epoll_fd[threadId], events, MAX_EVENTS, 1000);
        if (eventsize == -1) {
            continue;
        }

        for (int i = 0; i < eventsize; ++i) {
            int fd = events[i].data.fd;
            if (events[i].events & EPOLLOUT) {
                processSendQueue(fd, threadId);
            }
        }
    }
}

// 클라이언트 연결 수락 (Round-Robin 방식)
void Network::AcceptConnection(int worker_count) {
    struct epoll_event event;
    struct epoll_event events[MAX_EVENTS];
    struct sockaddr_in client_addr;
    std::atomic<int> next_recv_worker{ 0 };
    socklen_t client_len = sizeof(client_addr);

    while (running.load()) {
        int eventsize = epoll_wait(epoll_fd, events, MAX_EVENTS, 1000);
        if (eventsize == -1) {
            continue;
        }

        for (int i = 0; i < eventsize; ++i) {
            int fd = events[i].data.fd;
            if (fd == server_fd) {
                int clientsock =
                    accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
                if (clientsock == -1) {
                    continue;
                }

                // Round-Robin으로 워커 스레드에 분배
                int recv_worker_index = next_recv_worker.fetch_add(1) % worker_count;

                if (!set_nonblocking(clientsock)) {
                    close(clientsock);
                    continue;
                }

                // 수신 Epoll에 등록 (Edge-Triggered)
                event.events = EPOLLIN | EPOLLET;
                event.data.fd = clientsock;
                if (epoll_ctl(recv_epoll_fd[recv_worker_index], EPOLL_CTL_ADD,
                    clientsock, &event) == -1) {
                    close(clientsock);
                    continue;
                }
            }
        }
    }
}

// 송신 큐에 데이터 추가
void Network::addToSendQueue(int socket, bool isLast, std::vector<uint8_t>* data) {
    {
        std::lock_guard<std::mutex> lock(send_queue_mutex);
        send_queues[socket].push({ isLast,data });
    }

    // 송신 Epoll에 등록
    int send_worker_index = socket % send_epoll_fd.size();
    struct epoll_event event;
    event.events = EPOLLOUT | EPOLLET;
    event.data.fd = socket;

    if (epoll_ctl(send_epoll_fd[send_worker_index], EPOLL_CTL_ADD, socket,
        &event) == -1) {
        epoll_ctl(send_epoll_fd[send_worker_index], EPOLL_CTL_MOD, socket, &event);
    }
}

// 송신 큐 처리
void Network::processSendQueue(int fd, int threadId) {

    std::queue<std::pair<bool, std::vector<uint8_t>*>> local_queue;
    {
        std::lock_guard<std::mutex> lock(send_queue_mutex);
        auto it = send_queues.find(fd);
        if (it == send_queues.end() || it->second.empty()) {
            return;
        }

        local_queue.swap(it->second);
    }

    while (!local_queue.empty()) {
        auto data = local_queue.front();
        local_queue.pop();

        // RAII 패턴으로 메모리 자동 반환
        std::unique_ptr<std::vector<uint8_t>,
            std::function<void(std::vector<uint8_t>*)>>
            reguard(data.second, [](std::vector<uint8_t>* ptr) {
            G_MemoryPool->release<std::vector<uint8_t>>(ptr);
                });

        int sizes = data.second->size();
        int abc = send(fd, data.second->data(), data.second->size(), MSG_NOSIGNAL);
        if (abc <= 0)
        {
            printf("Socket error: %s (errno: %d)\n", strerror(errno), errno);
        }

        // 마지막 패킷이면 연결 종료
        if (data.first)
        {
            Disconnect(fd);
        }
    }
}

// 연결 종료 처리
void Network::Disconnect(int socket) {
    {
        std::lock_guard<std::mutex> lock2(send_queue_mutex);
        auto queue_it = send_queues.find(socket);
        if (queue_it != send_queues.end()) {
            while (!queue_it->second.empty()) {
                auto data = queue_it->second.front();
                queue_it->second.pop();
                G_MemoryPool->release<std::vector<uint8_t>>(data.second);
            }
            send_queues.erase(queue_it);
        }
    }

    // Epoll에서 제거
    for (size_t i = 0; i < recv_epoll_fd.size(); ++i) {
        epoll_ctl(recv_epoll_fd[i], EPOLL_CTL_DEL, socket, nullptr);
    }

    for (size_t i = 0; i < send_epoll_fd.size(); ++i) {
        epoll_ctl(send_epoll_fd[i], EPOLL_CTL_DEL, socket, nullptr);
    }

    close(socket);
}

// 클라이언트 연결 제거 (게임 로직 정리 포함)
void Network::removeConnection(int socket) {
    G_Routine->CleanupforSocket(socket);
    Disconnect(socket);
}