#include "MemoryPool.h"
#include "../GameCore/GameRoom/GameRoom.h"
#include "../GameCore/Player/Player.h"
#include "../Network/Packet/CompactBinaryReader/CompactBinaryReader.h"
#include <cstring>
#include <memory>
#include <mutex>
// 글로벌 메모리 풀 인스턴스 생성
std::unique_ptr<MemoryPool> G_MemoryPool = std::make_unique<MemoryPool>();
ClientPacket::ClientPacket() {};
ClientPacket::~ClientPacket() = default;
// 패킷 데이터 초기화
void ClientPacket::clear() {
  clientSocket = -1;
  data.clear();
}
MemoryPool::MemoryPool() {}

MemoryPool::~MemoryPool() {}
void MemoryPool::MemoryPool_Init(size_t packet_poolsize,
                                 size_t player_poolsize) {

  size_t vector_capacity = 1024;
  // 벡터 재할당 방지를 위한 공간 예약
  packet_owned_buffers.reserve(packet_poolsize);
  ClientPacket_owned_buffers.reserve(packet_poolsize);
  CompactBinaryReader_owned_buffers.reserve(packet_poolsize);
  Player_owned_buffers.reserve(player_poolsize);
  GameRoom_owned_buffers.reserve(50000);
  Room_Count = 50000;
  for (size_t i = 0; i < packet_poolsize; ++i) {
    // vector<uint8_t> 풀 초기화
    packet_owned_buffers.emplace_back(vector_capacity);
    packet_pool_storage.push(&packet_owned_buffers.back());
    // ClientPacket 풀 초기화
    ClientPacket_owned_buffers.emplace_back();
    ClientPacket_pool_storage.push(&ClientPacket_owned_buffers.back());
    // CompactBinaryReader 풀 초기화
    CompactBinaryReader_owned_buffers.emplace_back();
    CompactBinaryReader_pool_storage.push(&CompactBinaryReader_owned_buffers.back());
  }
  // Player 풀 초기화
  for (size_t i = 0; i < player_poolsize; ++i) {
    auto it = Player_owned_buffers.emplace(std::make_unique<Player>());
    Player_pool_storage.push(it.first->get());
  }
  // GameRoom 풀 초기화
  for (unsigned long long i = 0; i < Room_Count; ++i) {
    auto it = GameRoom_owned_buffers.emplace(std::make_unique<GameRoom>(i));
    GameRoom_pool_storage.push(it.first->get());
  }
}

// vector<uint8_t> 버퍼 할당
template <> std::vector<uint8_t> *MemoryPool::acquire<std::vector<uint8_t>>() {
  std::unique_lock<std::mutex> lock(packet_pool_mutex);
  packet_cv.wait(lock, [this] { return !packet_pool_storage.empty(); });
  auto buffer = packet_pool_storage.front();
  packet_pool_storage.pop();
  return buffer;
}

// ClientPacket 버퍼 할당
template <> ClientPacket *MemoryPool::acquire<ClientPacket>() {
  std::unique_lock<std::mutex> lock(ClientPacket_pool_mutex);
  ClientPacket_cv.wait(lock,
                       [this] { return !ClientPacket_pool_storage.empty(); });
  auto buffer = ClientPacket_pool_storage.front();
  ClientPacket_pool_storage.pop();
  return buffer;
}

// CompactBinaryReader 버퍼 할당
template <> CompactBinaryReader *MemoryPool::acquire<CompactBinaryReader>() {
  std::unique_lock<std::mutex> lock(CompactBinaryReader_pool_mutex);
  CompactBinaryReader_cv.wait(
      lock, [this] { return !CompactBinaryReader_pool_storage.empty(); });
  auto buffer = CompactBinaryReader_pool_storage.front();
  CompactBinaryReader_pool_storage.pop();
  return buffer;
}

// Player 버퍼 할당
template <> Player *MemoryPool::acquire<Player>() {
  std::unique_lock<std::mutex> lock(Player_pool_mutex);
  Player *buffer = nullptr;
  if (Player_pool_storage.empty()) {
    auto it = Player_owned_buffers.emplace(std::make_unique<Player>());
    buffer = it.first->get();
  } else {
    buffer = Player_pool_storage.front();
    Player_pool_storage.pop();
  }

  buffer->init_Player();
  return buffer;
}

// GameRoom 버퍼 할당
template <> GameRoom *MemoryPool::acquire<GameRoom>() {
  std::unique_lock<std::mutex> lock(GameRoom_pool_mutex);
  GameRoom *buffer = nullptr;
  if (GameRoom_pool_storage.empty()) {
    Room_Count++;
    auto it =
        GameRoom_owned_buffers.emplace(std::make_unique<GameRoom>(Room_Count));
    buffer = it.first->get();
  } else {
    buffer = GameRoom_pool_storage.front();
    GameRoom_pool_storage.pop();
  }
  buffer->init_GameRoom();

  return buffer;
}

// vector<uint8_t> 버퍼 반환
template <>
void MemoryPool::release<std::vector<uint8_t>>(std::vector<uint8_t> *buffer) {
  if (!buffer)
    return;
  buffer->clear();
  {
    std::lock_guard<std::mutex> lock(packet_pool_mutex);
    packet_pool_storage.push(buffer);
  }
  packet_cv.notify_one();
}

// ClientPacket 버퍼 반환
template <> void MemoryPool::release<ClientPacket>(ClientPacket *buffer) {
  if (!buffer)
    return;
  buffer->clear();
  {
    std::lock_guard<std::mutex> lock(ClientPacket_pool_mutex);
    ClientPacket_pool_storage.push(buffer);
  }
  ClientPacket_cv.notify_one();
}
// CompactBinaryReader 버퍼 반환
template <>
void MemoryPool::release<CompactBinaryReader>(CompactBinaryReader *buffer) {
  if (!buffer)
    return;
  buffer->clear();
  {
    std::lock_guard<std::mutex> lock(CompactBinaryReader_pool_mutex);
    CompactBinaryReader_pool_storage.push(buffer);
  }
  CompactBinaryReader_cv.notify_one();
}
// Player 버퍼 반환
template <> void MemoryPool::release<Player>(Player *buffer) {
  if (!buffer)
    return;

  buffer->Set_socket(-1);
  {
    std::lock_guard<std::mutex> lock(Player_pool_mutex);
    Player_pool_storage.push(buffer);
  }
}
// GameRoom 버퍼 반환
template <> void MemoryPool::release<GameRoom>(GameRoom *buffer) {
  if (!buffer)
    return;
  {
    std::lock_guard<std::mutex> lock(GameRoom_pool_mutex);
    GameRoom_pool_storage.push(buffer);
  }
}