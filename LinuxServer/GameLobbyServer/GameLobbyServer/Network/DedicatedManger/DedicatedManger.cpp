#include "DedicatedManger.h"
#include "../../MemoryPool/MemoryPool.h"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <vector>
#include <arpa/inet.h>
#include <sys/types.h>
#include <functional>
#include <unistd.h>
#include <iostream>

std::unique_ptr<DedicatedManger> G_DedicatedManger = std::make_unique<DedicatedManger>(9050);

DedicatedManger::DedicatedManger(uint16_t port)
    : Port(port) {
}

DedicatedManger::~DedicatedManger() { Release(); }

// 데디케이티드 서버 매니저 초기화
void DedicatedManger::initDedicatedManger() {
    struct epoll_event event;

    // 서버 소켓 생성
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        return;
    }

    // SO_REUSEADDR 옵션 설정
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(server_fd);
        server_fd = -1;
        return;
    }

    // 서버 주소 설정
    int serveraddrsize = sizeof(server_addr);
    std::memset(&server_addr, 0x0, serveraddrsize);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(Port);

    // 바인드
    if (bind(server_fd, (struct sockaddr*)&server_addr, serveraddrsize)) {
        close(server_fd);
        server_fd = -1;
        return;
    }

    // 리슨
    if (listen(server_fd, SOMAXCONN)) {
        close(server_fd);
        server_fd = -1;
        return;
    }

    // 논블로킹 설정
    if (!set_nonblocking(server_fd)) {
        close(server_fd);
        server_fd = -1;
        return;
    }

    // Epoll 생성 및 서버 소켓 등록
    epoll_fd = epoll_create(1);
    if (epoll_fd == -1) {
        close(server_fd);
        server_fd = -1;
        return;
    }

    event.events = EPOLLIN;
    event.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, event.data.fd, &event) == -1) {
        close(server_fd);
        close(epoll_fd);
        server_fd = -1;
        epoll_fd = -1;
        return;
    }
    return;
}

// 네트워크 시작 및 워커 스레드 생성
void DedicatedManger::StartNetwork(unsigned int recvthreadcount,
    unsigned int sendthreadcount) {

    running.store(true);

    send_epoll_fd.resize(sendthreadcount);
    recv_epoll_fd.resize(recvthreadcount);

    // 수신 워커 스레드 생성
    for (unsigned int i = 0; i < recvthreadcount; ++i) {
        recv_epoll_fd[i] = epoll_create(1);
        if (recv_epoll_fd[i] == -1) {
            Release();
            return;
        }
        recvThreads.emplace_back(&DedicatedManger::recvWorkerThread, this, i);
    }

    // 송신 워커 스레드 생성
    for (unsigned int i = 0; i < sendthreadcount; ++i) {
        send_epoll_fd[i] = epoll_create(1);
        if (send_epoll_fd[i] == -1) {
            Release();
            return;
        }
        sendThreads.emplace_back(&DedicatedManger::sendWorkerThread, this, i);
    }

    AcceptConnection(recvthreadcount);
    return;
}

// 리소스 정리
void DedicatedManger::Release() {

    if (!running.load())
        return;

    running.store(false);

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
bool DedicatedManger::set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

// 수신 워커 스레드 (현재 미사용)
void DedicatedManger::recvWorkerThread(int threadId) {
    struct epoll_event events[MAX_EVENTS];
    while (running.load()) {
    }
}

// 송신 워커 스레드
void DedicatedManger::sendWorkerThread(int threadId) {
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

// 사용 가능한 데디케이티드 서버 조회
std::pair<int, DedicatedServerInfor*> DedicatedManger::GetDedicatedServerInfor()
{
    for (auto& it : serverlist)
    {
        if (!it.second.used)
        {
            it.second.used = true;
            return { it.first,&it.second };
        }
    }
    return { -1,nullptr };
}

// 데디케이티드 서버 연결 수락
void DedicatedManger::AcceptConnection(int worker_count) {
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
                int Dedicated =
                    accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
                if (Dedicated == -1) {
                    continue;
                }

                int recv_worker_index = next_recv_worker.fetch_add(1) % worker_count;

                if (!set_nonblocking(Dedicated)) {
                    close(Dedicated);
                    continue;
                }

                event.events = EPOLLIN | EPOLLET;
                event.data.fd = Dedicated;
                if (epoll_ctl(recv_epoll_fd[recv_worker_index], EPOLL_CTL_ADD,
                    Dedicated, &event) == -1) {
                    close(Dedicated);
                    continue;
                }

                // 연결된 데디케이티드 서버 정보 저장
                char ip_buf[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client_addr.sin_addr, ip_buf, sizeof(ip_buf));
                serverlist[Dedicated] = DedicatedServerInfor(ip_buf, 7777);
                std::cout << '\n' << "[GameLobbyServer]" << "새로운 데디게이트서버 :" << ip_buf << " 를 추가 하였습니다  \n";
            }
        }
    }
}

// 송신 큐에 데이터 추가
void DedicatedManger::addToSendQueue(int socket, bool isLast,
    std::vector<uint8_t>* data) {

        {
            std::lock_guard<std::mutex> lock(send_queue_mutex);
            send_queues[socket].push({ isLast, data });
        }

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
void DedicatedManger::processSendQueue(int sock, int threadId) {
    std::queue<std::pair<bool, std::vector<uint8_t>*>> local_queue;
    {
        std::lock_guard<std::mutex> lock(send_queue_mutex);
        auto it = send_queues.find(sock);
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
        int abc = send(sock, data.second->data(), data.second->size(), MSG_NOSIGNAL);
        if (abc <= 0)
        {
            // 전송 실패 처리
        }
    }
}

// 연결 종료 처리
void DedicatedManger::Disconnect(int socket) {
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

    for (size_t i = 0; i < recv_epoll_fd.size(); ++i) {
        epoll_ctl(recv_epoll_fd[i], EPOLL_CTL_DEL, socket, nullptr);
    }

    for (size_t i = 0; i < send_epoll_fd.size(); ++i) {
        epoll_ctl(send_epoll_fd[i], EPOLL_CTL_DEL, socket, nullptr);
    }

    close(socket);
}

void DedicatedManger::removeConnection(int socket) {
    Disconnect(socket);
}