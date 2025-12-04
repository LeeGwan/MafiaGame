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
  // 메모리 풀 초기화 (버퍼 수량 설정)
  void Init_MemoryPool(size_t size);
  // 버퍼 요청
  template<typename T>
  T* acquire();
  template<typename T>
  // 버퍼 반환
  void release(T* buffer);
private:
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

  size_t pool_size;
  size_t vector_capacity;
};
// 글로벌 메모리풀 객체
extern std::unique_ptr<MemoryPool> G_MemoryPool;
#endif