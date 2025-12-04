#ifndef DEDICATEDMANGER_H
#define DEDICATEDMANGER_H

#pragma once
#include <string>
#include <cstdint>
#include <queue>
#include <atomic>
#include <unordered_map>
#include <unordered_set>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <memory>
#include <mutex>
#include <thread>
#include <netinet/in.h>

// 데디케이티드 서버 정보
struct DedicatedServerInfor
{
    std::string IP;
    uint16_t Port;
    bool used;

    DedicatedServerInfor() : IP(), Port(0), used(false) {}

    DedicatedServerInfor(const std::string& ip, uint16_t port)
        : IP(ip), Port(port), used(false) {
    }

    bool operator==(const DedicatedServerInfor& other) const {
        return IP == other.IP && Port == other.Port;
    }
};

// 데디케이티드 게임 서버 관리 시스템
class DedicatedManger
{
public:
    DedicatedManger(uint16_t port);
    ~DedicatedManger();
    void initDedicatedManger();
    void StartNetwork(unsigned int recvthreadcount, unsigned int sendthreadcount);
    void addToSendQueue(int socket, bool isLast, std::vector<uint8_t>* data);
    void Disconnect(int socket);
    void Release();
    std::pair<int, DedicatedServerInfor*> GetDedicatedServerInfor(); // 사용 가능한 데디서버 조회

private:
    uint16_t Port;
    std::unordered_map<int, DedicatedServerInfor> serverlist; // 연결된 데디서버 목록

    const int MAX_EVENTS = 1024;
    const int BUFFER_SIZE = 1024;
    std::atomic<bool> running;
    int port;
    int server_fd;
    int epoll_fd;
    struct sockaddr_in server_addr;

    std::vector<int> recv_epoll_fd;
    std::vector<std::thread> recvThreads;

    std::vector<int> send_epoll_fd;
    std::vector<std::thread> sendThreads;

    std::unordered_map<int, std::queue<std::pair<bool, std::vector<uint8_t>*>>> send_queues;
    std::mutex send_queue_mutex;

private:
    bool set_nonblocking(int fd);
    void recvWorkerThread(int threadId);
    void sendWorkerThread(int threadId);
    void AcceptConnection(int worker_count);
    void processSendQueue(int sock, int threadId);
    void removeConnection(int socket);
};

extern std::unique_ptr<DedicatedManger> G_DedicatedManger;

#endif