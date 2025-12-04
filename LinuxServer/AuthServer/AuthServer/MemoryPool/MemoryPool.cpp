#include "MemoryPool.h"
#include <cstring> 
#include <mutex>
#include "../Network/Packet/CompactBinaryReader/CompactBinaryReader.h"
// 글로벌 메모리 풀 인스턴스 생성
std::unique_ptr<MemoryPool> G_MemoryPool=std::make_unique<MemoryPool>();
ClientPacket::ClientPacket(){};
ClientPacket::~ClientPacket()=default;
// 패킷 데이터 초기화
void ClientPacket::clear()
{
    
    clientSocket=-1;
    data.clear();
}
MemoryPool::MemoryPool(){}

MemoryPool::~MemoryPool()
{
    
   
}
void MemoryPool::Init_MemoryPool(size_t size)
{
    pool_size = size;
  vector_capacity = 1024;
  // 벡터 재할당 방지를 위한 공간 예약
  packet_owned_buffers.reserve(pool_size);
  ClientPacket_owned_buffers.reserve(pool_size);
  CompactBinaryReader_owned_buffers.reserve(pool_size);

  for (size_t i = 0; i < pool_size; ++i) {
    // vector<uint8_t> 풀 초기화
    packet_owned_buffers.emplace_back(vector_capacity);
    packet_pool_storage.push(&packet_owned_buffers.back());
    // ClientPacket 풀 초기화
    ClientPacket_owned_buffers.emplace_back();
    ClientPacket_pool_storage.push(&ClientPacket_owned_buffers.back());
    // CompactBinaryReader 풀 초기화
    CompactBinaryReader_owned_buffers.emplace_back();
    CompactBinaryReader_pool_storage.push( &CompactBinaryReader_owned_buffers.back());
  }
}
// vector<uint8_t> 버퍼 할당
template <>
std::vector<uint8_t>* MemoryPool::acquire<std::vector<uint8_t>>()
{
    std::unique_lock<std::mutex> lock(packet_pool_mutex);
    packet_cv.wait(lock, [this] { return !packet_pool_storage.empty(); });
    auto buffer = packet_pool_storage.front();
    packet_pool_storage.pop();
    return buffer;
}
// ClientPacket 버퍼 할당
template <>
ClientPacket* MemoryPool::acquire<ClientPacket>()
{
    std::unique_lock<std::mutex> lock(ClientPacket_pool_mutex);
   ClientPacket_cv.wait(lock, [this] { return !ClientPacket_pool_storage.empty(); });
    auto buffer = ClientPacket_pool_storage.front();
    ClientPacket_pool_storage.pop();
    return buffer;
}
// CompactBinaryReader 버퍼 할당
template <>
CompactBinaryReader* MemoryPool::acquire<CompactBinaryReader>()
{
    std::unique_lock<std::mutex> lock(CompactBinaryReader_pool_mutex);
    CompactBinaryReader_cv.wait(lock, [this] { return !CompactBinaryReader_pool_storage.empty(); });
    auto buffer = CompactBinaryReader_pool_storage.front();
    CompactBinaryReader_pool_storage.pop();
    return buffer;
}
// vector<uint8_t> 버퍼 반환
template <>
void MemoryPool::release<std::vector<uint8_t>>(std::vector<uint8_t>* buffer)
{
    if (!buffer) return;
    buffer->clear();
    {
        std::lock_guard<std::mutex> lock(packet_pool_mutex);
        packet_pool_storage.push(buffer);
    }
    packet_cv.notify_one();
}
// ClientPacket 버퍼 반환
template <>
void MemoryPool::release<ClientPacket>(ClientPacket* buffer)
{
    if (!buffer) return;
    buffer->clear();
    {
        std::lock_guard<std::mutex> lock(ClientPacket_pool_mutex);
        ClientPacket_pool_storage.push(buffer);
    }
    ClientPacket_cv.notify_one();
}
// CompactBinaryReader 버퍼 반환
template <>
void MemoryPool::release<CompactBinaryReader>(CompactBinaryReader* buffer)
{
    if (!buffer) return;
    buffer->clear();
    {
        std::lock_guard<std::mutex> lock(CompactBinaryReader_pool_mutex);
        CompactBinaryReader_pool_storage.push(buffer);
    }
   CompactBinaryReader_cv.notify_one();
}