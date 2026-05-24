// Fill out your copyright notice in the Description page of Project Settings.

#include "MemoryPool.h"
#include <cstring> 
#include <mutex>
#include "../Network/Packet/CompactBinaryReader/CompactBinaryReader.h"

/** Global Memory Pool instance for centralized resource management */
std::unique_ptr<MemoryPool> G_MemoryPool = std::make_unique<MemoryPool>();

ClientPacket::ClientPacket() {}
ClientPacket::~ClientPacket() = default;

/**
 * @brief Resets the packet state for reuse. 
 * Essential to prevent data leakage between different client sessions.
 */
void ClientPacket::clear()
{
    clientSocket = -1;
    data.clear();
}

MemoryPool::MemoryPool() {}
MemoryPool::~MemoryPool() {}

/**
 * @brief Initializes the memory pools with a fixed size to prevent runtime heap allocation.
 * Pre-allocates buffers for raw data, client packets, and binary readers.
 * @param size The total number of objects to pre-allocate for each pool type.
 */
void MemoryPool::Init_MemoryPool(size_t size)
{
    pool_size = size;
    vector_capacity = 1024;

    // Reserve capacity to prevent vector reallocations during initialization
    packet_owned_buffers.reserve(pool_size);
    ClientPacket_owned_buffers.reserve(pool_size);
    CompactBinaryReader_owned_buffers.reserve(pool_size);

    for (size_t i = 0; i < pool_size; ++i) 
    {
        // Initialize Raw Byte Buffer Pool
        packet_owned_buffers.emplace_back(vector_capacity);
        packet_pool_storage.push(&packet_owned_buffers.back());

        // Initialize ClientPacket Object Pool
        ClientPacket_owned_buffers.emplace_back();
        ClientPacket_pool_storage.push(&ClientPacket_owned_buffers.back());

        // Initialize CompactBinaryReader Object Pool
        CompactBinaryReader_owned_buffers.emplace_back();
        CompactBinaryReader_pool_storage.push(&CompactBinaryReader_owned_buffers.back());
    }
}

/**
 * @section Acquire_Specializations
 * Thread-safe retrieval of objects from their respective pools.
 * Uses condition variables to block the thread if the pool is temporarily exhausted.
 */

// Acquire: std::vector<uint8_t>
template <>
std::vector<uint8_t>* MemoryPool::acquire<std::vector<uint8_t>>()
{
    std::unique_lock<std::mutex> lock(packet_pool_mutex);
    packet_cv.wait(lock, [this] { return !packet_pool_storage.empty(); });
    
    auto buffer = packet_pool_storage.front();
    packet_pool_storage.pop();
    return buffer;
}

// Acquire: ClientPacket
template <>
ClientPacket* MemoryPool::acquire<ClientPacket>()
{
    std::unique_lock<std::mutex> lock(ClientPacket_pool_mutex);
    ClientPacket_cv.wait(lock, [this] { return !ClientPacket_pool_storage.empty(); });
    
    auto buffer = ClientPacket_pool_storage.front();
    ClientPacket_pool_storage.pop();
    return buffer;
}

// Acquire: CompactBinaryReader
template <>
CompactBinaryReader* MemoryPool::acquire<CompactBinaryReader>()
{
    std::unique_lock<std::mutex> lock(CompactBinaryReader_pool_mutex);
    CompactBinaryReader_cv.wait(lock, [this] { return !CompactBinaryReader_pool_storage.empty(); });
    
    auto buffer = CompactBinaryReader_pool_storage.front();
    CompactBinaryReader_pool_storage.pop();
    return buffer;
}

/**
 * @section Release_Specializations
 * Returns objects to the pool after resetting their state.
 * Notifies waiting threads that a resource has become available.
 */

// Release: std::vector<uint8_t>
template <>
void MemoryPool::release<std::vector<uint8_t>>(std::vector<uint8_t>* buffer)
{
    if (!buffer) return;
    buffer->clear(); // Ensure the buffer is wiped before returning to the pool
    
    {
        std::lock_guard<std::mutex> lock(packet_pool_mutex);
        packet_pool_storage.push(buffer);
    }
    packet_cv.notify_one();
}

// Release: ClientPacket
template <>
void MemoryPool::release<ClientPacket>(ClientPacket* buffer)
{
    if (!buffer) return;
    buffer->clear(); // Reset socket and data for the next session
    
    {
        std::lock_guard<std::mutex> lock(ClientPacket_pool_mutex);
        ClientPacket_pool_storage.push(buffer);
    }
    ClientPacket_cv.notify_one();
}

// Release: CompactBinaryReader
template <>
void MemoryPool::release<CompactBinaryReader>(CompactBinaryReader* buffer)
{
    if (!buffer) return;
    buffer->clear(); // Reset reader internal offsets
    
    {
        std::lock_guard<std::mutex> lock(CompactBinaryReader_pool_mutex);
        CompactBinaryReader_pool_storage.push(buffer);
    }
    CompactBinaryReader_cv.notify_one();
}
