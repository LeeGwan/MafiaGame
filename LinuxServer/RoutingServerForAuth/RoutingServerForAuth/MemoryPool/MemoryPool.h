// Fill out your copyright notice in the Description page of Project Settings.

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

/**
 * @struct ClientPacket
 * @brief Data structure for storing raw packet payloads associated with a specific client socket.
 */
struct ClientPacket {
    int clientSocket;
    std::vector<uint8_t> data;

    ClientPacket();
    ~ClientPacket();

    /** @brief Resets socket descriptor and clears data buffer for object reuse. */
    void clear();
};

/**
 * @class MemoryPool
 * @brief Authoritative resource manager that implements Object Pooling to mitigate heap fragmentation.
 * * Provides thread-safe allocation and deallocation for raw buffers, packet containers, 
 * and binary readers using pre-allocated memory segments.
 */
class MemoryPool {
public:
    MemoryPool();
    ~MemoryPool();

    /**
     * @brief Pre-allocates memory for all internal pools to a fixed capacity.
     * @param size The number of objects to pre-initialize in each pool.
     */
    void Init_MemoryPool(size_t size);

    /**
     * @brief Acquires a pointer to a pre-allocated object of type T.
     * This operation is thread-safe and may block if the pool is empty.
     */
    template<typename T>
    T* acquire();

    /**
     * @brief Returns a borrowed object of type T back to the pool after state sanitization.
     * @param buffer Pointer to the object being released.
     */
    template<typename T>
    void release(T* buffer);

private:
    // === Network Resource Pools ===

    /** @section Raw_Byte_Buffer_Pool (std::vector<uint8_t>) */
    std::queue<std::vector<uint8_t>*> packet_pool_storage;         // Queue of available pointers
    std::vector<std::vector<uint8_t>> packet_owned_buffers;       // Contiguous memory ownership
    std::mutex packet_pool_mutex;
    std::condition_variable packet_cv;

    /** @section ClientPacket_Object_Pool */
    std::queue<ClientPacket*> ClientPacket_pool_storage;          // Queue of available pointers
    std::vector<ClientPacket> ClientPacket_owned_buffers;         // Contiguous memory ownership
    std::mutex ClientPacket_pool_mutex;
    std::condition_variable ClientPacket_cv;

    /** @section CompactBinaryReader_Object_Pool */
    std::queue<CompactBinaryReader*> CompactBinaryReader_pool_storage;
    std::vector<CompactBinaryReader> CompactBinaryReader_owned_buffers; // Contiguous memory ownership
    std::mutex CompactBinaryReader_pool_mutex;
    std::condition_variable CompactBinaryReader_cv;

    /** Pool Configuration */
    size_t pool_size;
    size_t vector_capacity;
};

/** Global access pointer for the thread-safe MemoryPool singleton. */
extern std::unique_ptr<MemoryPool> G_MemoryPool;

#endif
