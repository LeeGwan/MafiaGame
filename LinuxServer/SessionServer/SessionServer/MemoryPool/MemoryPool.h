#ifndef MEMORYPOOL_H
#define MEMORYPOOL_H
#pragma once
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <stdint.h>
#include <vector>
#include <netinet/in.h>
class CompactBinaryReader;
class ClientInfo;
// 클라이언트 패킷 데이터 저장 구조체
struct ClientPacket {
    int clientSocket;
    std::vector<uint8_t> data;
    ClientPacket();
    ~ClientPacket();
    // 패킷 데이터와 소켓 초기화
    void clear();
};
// 메모리 풀 관리 클래스
class MemoryPool {
public:
    MemoryPool();
    ~MemoryPool();
    //메모리 초기화
    void MemoryPool_Init(size_t packet_poolsize, size_t player_poolsize);
    // 메모리 할당
    template<typename T>
    T* acquire();
    // 메모리 반환
    template<typename T>
    void release(T* buffer);
private:
    // === 네트워크 메모리 풀 ===

    // std::vector<uint8_t> 메모리 풀
    std::queue<std::vector<uint8_t>*> packet_pool_storage;
    std::vector<std::vector<uint8_t>> packet_owned_buffers;
    std::mutex packet_pool_mutex;
    std::condition_variable packet_cv;

    // ClientPacket 메모리 풀
    std::queue<ClientPacket*> ClientPacket_pool_storage;
    std::vector<ClientPacket> ClientPacket_owned_buffers;
    std::mutex ClientPacket_pool_mutex;
    std::condition_variable ClientPacket_cv;

    // CompactBinaryReader 메모리 풀
    std::queue<CompactBinaryReader*> CompactBinaryReader_pool_storage;
    std::vector<CompactBinaryReader> CompactBinaryReader_owned_buffers;
    std::mutex CompactBinaryReader_pool_mutex;
    std::condition_variable CompactBinaryReader_cv;

    // ClientInfo 메모리 풀
    std::queue<ClientInfo*> ClientInfo_pool_storage;
    std::vector<ClientInfo> ClientInfo_buffers;  
    std::mutex ClientInfo_pool_mutex;
    std::condition_variable ClientInfo_cv;

    size_t pool_size;
    size_t vector_capacity;
};
// 글로벌 메모리풀 객체
extern std::unique_ptr<MemoryPool> G_MemoryPool;
#endif