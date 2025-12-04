#include "ServerRegistry.h"
#include "../../MemoryPool/MemoryPool.h"
#include "../Aes/Aes.h"
#include "../Packet/CompactBinaryReader/CompactBinaryReader.h"
#include "../Packet/OptimizedBinaryPacketSerializer/OptimizedBinaryPacketSerializer.h"
#include "../Packet/PacketStructure/PacketStructure.h"
#include "../ServerInfo/ServerInfo.h"
#include <cstring>
#include <fcntl.h>
#include <functional>
#include <memory>
#include <mutex>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <iostream>

std::unique_ptr<ServerRegistry> G_ServerRegistry =
std::make_unique<ServerRegistry>();

ServerRegistry::ServerRegistry() : port(8010), server_fd(-1), epoll_fd(-1) {}

ServerRegistry::~ServerRegistry() { Release(); }

// 논블로킹 소켓 설정
bool ServerRegistry::set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

// 서버 레지스트리 초기화
bool ServerRegistry::Init() {
    if (!ConnectSession())
        return false;

    return true;
}

// 네트워크 시작 및 워커 스레드 생성
void ServerRegistry::StartNetwork(unsigned int threadcount) {
    running.store(true);
    epolls_fd.resize(threadcount);

    // 수신 워커 스레드 생성
    for (unsigned int i = 0; i < threadcount; ++i) {
        epolls_fd[i] = epoll_create(1);
        if (epolls_fd[i] == -1) {
            Release();
            return;
        }
        recvThreads.emplace_back(&ServerRegistry::recvWorkerThread, this, i);
    }

    // 하트비트 체크 스레드 생성
    for (unsigned int i = 0; i < threadcount; ++i) {
        sendThreads.emplace_back(&ServerRegistry::sendWorkerThread, this, i);
    }

    AcceptConnection(threadcount);
    return;
}

// 리소스 정리
void ServerRegistry::Release() {
    if (!running.load())
        return;
    running.store(false);

    if (server_fd != -1) {
        close(server_fd);
        server_fd = -1;
    }

    if (epoll_fd != -1) {
        close(epoll_fd);
        epoll_fd = -1;
    }

    for (auto& thread : recvThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    for (auto& thread : sendThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    // 등록된 서버 정리
    {
        std::lock_guard<std::mutex> lock(ServerlistMTX);
        for (const auto& [socket, serverInfo] : Serverlist) {
            for (int epoll_fd : epolls_fd) {
                if (epoll_fd != -1) {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, socket, nullptr);
                }
            }
            close(socket);
        }
        Serverlist.clear();
    }

    for (int& epoll_fd : epolls_fd) {
        if (epoll_fd != -1) {
            close(epoll_fd);
            epoll_fd = -1;
        }
    }

    epolls_fd.clear();
    recvThreads.clear();
    sendThreads.clear();
}

// 서버 소켓 생성 및 바인드
bool ServerRegistry::ConnectSession() {
    struct epoll_event event;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        return false;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(server_fd);
        server_fd = -1;
        return false;
    }

    int serveraddrsize = sizeof(server_addr);
    std::memset(&server_addr, 0x0, serveraddrsize);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

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

    if (!set_nonblocking(server_fd)) {
        close(server_fd);
        server_fd = -1;
        return false;
    }

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

// 로드밸런싱: Round-Robin 방식으로 서버 선택
bool ServerRegistry::SelectServer(std::string& Ip, uint16_t& port) {

    std::lock_guard<std::mutex> lock1(ServerlistMTX);
    if (Serverlist.empty()) {
        return false;
    }

    size_t index = currentIndex.fetch_add(1) % Serverlist.size();

    // unordered_map의 index번째 요소 찾기
    auto it = Serverlist.begin();
    std::advance(it, index);

    // 선택된 서버가 유효하고 사용 가능한 상태인지 확인
    if (it->second && it->second->Isused()) {
        auto [serverip, serverport] = it->second->Get_serverInfor();
        Ip = serverip;
        port = serverport;
        std::cout << '\n' << "[AuthRoutineServer]" << "관리중인 서버 갯수: " << Serverlist.size() << "서버 정보 아이피:" << Ip << "를 제공\n";
    }

    return true;
}

// 게임 서버 연결 수락
void ServerRegistry::AcceptConnection(int worker_count) {
    static int c = 0;
    struct epoll_event event;
    struct epoll_event events[MAX_EVENTS];
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    std::atomic<int> next_recv_worker{ 0 };

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
                c++;

                int recv_worker_index = next_recv_worker.fetch_add(1) % worker_count;

                if (!set_nonblocking(clientsock)) {
                    close(clientsock);
                    continue;
                }

                event.events = EPOLLIN | EPOLLET;
                event.data.fd = clientsock;
                if (epoll_ctl(epolls_fd[recv_worker_index], EPOLL_CTL_ADD, clientsock,
                    &event) == -1) {
                    close(clientsock);
                    continue;
                }
            }
        }
    }
}

// 수신 워커 스레드
void ServerRegistry::recvWorkerThread(int threadId) {
    struct epoll_event events[MAX_EVENTS];
    std::vector<uint8_t>* packet = nullptr;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    while (running.load()) {
        int eventsize = epoll_wait(epolls_fd[threadId], events, MAX_EVENTS, 1000);
        if (eventsize == -1) {
            continue;
        }

        for (int i = 0; i < eventsize; ++i) {
            int fd = events[i].data.fd;
            if (events[i].events & EPOLLIN) {
                packet = G_MemoryPool->acquire<std::vector<uint8_t>>();
                packet->resize(BUFFER_SIZE);

                ssize_t bytes_received = recv(fd, packet->data(), BUFFER_SIZE, 0);
                if (bytes_received > 0) {
                    try {
                        packet->resize(bytes_received);
                        ProcessPacket(fd, packet);
                    }
                    catch (const std::exception& e) {
                        G_MemoryPool->release<std::vector<uint8_t>>(packet);
                        removeConnection(fd);
                    }
                }
                else if (bytes_received == 0) {
                    G_MemoryPool->release<std::vector<uint8_t>>(packet);
                    removeConnection(fd);
                }
                else {
                    if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        G_MemoryPool->release<std::vector<uint8_t>>(packet);
                        removeConnection(fd);
                    }
                }
            }
        }
    }
}

// 하트비트 체크 스레드
void ServerRegistry::sendWorkerThread(int threadId) {
    std::vector<ServerInfo*> socketsToRemove;
    std::vector<std::pair<int, ServerInfo*>> List;
    List.reserve(MAX_EVENTS);

    while (running.load()) {
        {
            std::lock_guard<std::mutex> lock(ServerlistMTX);
            auto it = Serverlist.begin();
            while (it != Serverlist.end()) {
                if (!it->second || !it->second->Isused()) {
                    // 하트비트 타임아웃시 서버 연결종료
                    it = Serverlist.erase(it);
                }
                else {
                    List.push_back({ it->first, it->second.get() });
                    ++it;
                }
            }
        }

        if (!List.empty()) {
            for (const auto& [key, value] : List) {
                auto heartbeatTimeout = std::chrono::seconds(400);
                auto heartbeatTime = std::chrono::seconds(240);
                auto now = std::chrono::steady_clock::now();
                auto heartbeat = value->Get_lastHeartbeat();

                // 하트비트 시간 체크
                if (now - heartbeat > heartbeatTime && !value->IsSendHeartbeat()) {
                    value->SetsendHeartbeat(true);
                    SendHeartbeat(key);
                    std::cout << '\n' << "[AuthRoutineServer]" << value->Get_serverInfor().first << "하트 비트 전송\n";
                }

                if (now - heartbeat > heartbeatTimeout) {
                    // 타임아웃
                    socketsToRemove.push_back(value);
                }
            }

            for (ServerInfo* server : socketsToRemove) {
                server->Setused(false);
            }

            socketsToRemove.clear();
            List.clear();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// 게임 서버로부터 받은 패킷 처리
void ServerRegistry::ProcessPacket(int clientsocket,
    std::vector<uint8_t>* data) {
    try {
        if (!AES)
            return;

        CompactBinaryReader* reader = nullptr;
        std::vector<uint8_t>* decrypted_data =
            G_MemoryPool->acquire<std::vector<uint8_t>>();

        // AES 복호화
        AES->Aes_Decrypt(data, decrypted_data);
        reader = G_MemoryPool->acquire<CompactBinaryReader>();
        PacketType packet_type;

        G_MemoryPool->release<std::vector<uint8_t>>(data);

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

        switch (packet_type) {
        case PacketType::ConnectionCheck: {
            ServerInfoPacket information_packet;
            // 역직렬화
            OptimizedBinaryPacketSerializer::DeserializePacket<ServerInfoPacket>(
                *reader, information_packet);

            if (information_packet.IP.empty())
                return;

            {
                std::lock_guard<std::mutex> lock(ServerlistMTX);
                // 서버목록에 할당
                Serverlist.emplace(clientsocket, std::make_unique<ServerInfo>());
                // 서버 등록
                Serverlist[clientsocket]->RegisterServerInfo(information_packet.IP, information_packet.port);
                std::cout << '\n' << "[AuthRoutineServer]" << "새로운 서버 :" << information_packet.IP << " 를 추가 하였습니다  \n";
            }
            break;
        }
        case PacketType::Heartbeat: {
            {
                std::lock_guard<std::mutex> lock(ServerlistMTX);
                Serverlist[clientsocket]->Set_lastHeartbeat(
                    std::chrono::steady_clock::now());
                std::cout << '\n' << "[AuthRoutineServer]" << "서버 :" << Serverlist[clientsocket]->Get_serverInfor().first << " 하트비트 성공 \n";
            }
            break;
        }
        default:
            break;
        }
    }
    catch (...) {
        return;
    }
}

// 서버 연결 제거
void ServerRegistry::removeConnection(int socket) {
    for (size_t i = 0; i < epolls_fd.size(); ++i) {
        epoll_ctl(epolls_fd[i], EPOLL_CTL_DEL, socket, nullptr);
    }

    {
        std::lock_guard<std::mutex> lock1(ServerlistMTX);
        auto it = Serverlist.find(socket);
        if (it != Serverlist.end()) {
            it->second->Setused(false);
        }
    }

    close(socket);
}

// 게임 서버에 하트비트 전송
void ServerRegistry::SendHeartbeat(int clientsocket) {
    try {
        if (!AES)
            return;

        std::vector<uint8_t>* heartbeat_data =
            G_MemoryPool->acquire<std::vector<uint8_t>>();
        std::vector<uint8_t>* encrypted_data =
            G_MemoryPool->acquire<std::vector<uint8_t>>();

        // RAII 가드 설정
        std::unique_ptr<std::vector<uint8_t>,
            std::function<void(std::vector<uint8_t>*)>>
            heartbeat_guard(heartbeat_data, [](std::vector<uint8_t>* p) {
            G_MemoryPool->release<std::vector<uint8_t>>(p);
                });
        std::unique_ptr<std::vector<uint8_t>,
            std::function<void(std::vector<uint8_t>*)>>
            encrypted_guard(encrypted_data, [](std::vector<uint8_t>* p) {
            G_MemoryPool->release<std::vector<uint8_t>>(p);
                });

        TypePacket heartbeat_packet;
        heartbeat_packet.Type = PacketType::Heartbeat;
        OptimizedBinaryPacketSerializer::SerializePacket<TypePacket>(
            heartbeat_packet, heartbeat_data);

        AES->Aes_Encrypt(heartbeat_data, encrypted_data);

        send(clientsocket, encrypted_data->data(), encrypted_data->size(),
            MSG_NOSIGNAL);

    }
    catch (...) {
    }
}