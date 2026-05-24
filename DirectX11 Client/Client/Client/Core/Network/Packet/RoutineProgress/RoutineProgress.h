/**
 * @file RoutineProgress.h
 * @brief Header for the asynchronous packet processing engine and thread-pool orchestration.
 */

#pragma once
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <stdint.h>
#include <unordered_map>
#include <string>
#include <thread>
#include <vector>

// Forward Declarations
class Aes;                           
class MafiaDatabase;                 
class CompactBinaryReader;           
enum class ResultType : uint8_t;     
enum class PacketType : uint8_t;     
struct TypePacket;                   
struct ResultPacket;                 
struct ResultAndHashPacket;          
struct HashPacket;                   
struct ServerInfoPacket;             
struct ClientPacket;                 
struct IntegrityCheckPacket;         
struct stringforVectorPacket;        

/**
 * @class RoutineProgress
 * @brief Manages a worker thread pool for asynchronous packet decryption, deserialization, and routing.
 * * This class implements a Producer-Consumer pattern to decouple network I/O from 
 * application logic, ensuring the UI thread remains responsive during heavy network traffic.
 */
class RoutineProgress {
public:
    /**
     * @brief Constructor: Initializes the worker thread pool.
     * @param in_threadcount Number of concurrent threads to spawn for packet processing.
     */
    RoutineProgress(uint8_t in_threadcount);
    ~RoutineProgress();

    // Deleted copy semantics to enforce Singleton/Unique ownership
    RoutineProgress(const RoutineProgress&) = delete;
    RoutineProgress& operator=(const RoutineProgress&) = delete;

    // --- Specialized Transmission Dispatchers ---

    /** @brief Dispatches a header-only packet (Type only). */
    void SendResponseForTypePacket(PacketType type);

    /** @brief Dispatches a dual-string packet (Used for Authentication/Registration). */
    void SendResponseForTwoStringPacket(PacketType type, const std::string& str1, const std::string& str2);

    /** @brief Dispatches a session hash packet for identity verification. */
    void SendResponseForHashPacket(PacketType type, const std::string& str1);

    /** @brief Dispatches hardware-level integrity data for anti-cheat verification. */
    void SendResponseForIntegrityCheckPacket(const IntegrityCheckPacket& packet);

    /** @brief Dispatches a collection of strings (e.g., lobby lists or player names). */
    void SendResponseForstringforVectorPacket(const stringforVectorPacket& packet);

    /** @brief Dispatches a standardized result code packet. */
    void SendResponseForstringforResultPacket(const ResultPacket& packet);

    /**
     * @brief Dispatches a high-priority packet that bypasses the asynchronous queue.
     * @return Raw encrypted ciphertext for immediate network transmission.
     */
    std::vector<uint8_t> SendResponseForpriorityPacket(PacketType type, const std::string& str1);

    /**
     * @brief Enqueues raw network data into the processing queue.
     * @note Typically called by the Network Receiver thread.
     */
    void addToProgressQueue(const std::vector<uint8_t>& data);

    /** @brief Gracefully terminates all worker threads and cleans up resources. */
    void Release();

private:
    uint8_t threadcount;                          
    std::mutex client_sockets_mutex;              
    std::vector<std::thread> ProsessThreads;      

    /** @section Task_Orchestration 
     * Internal queue and synchronization primitives for the Producer-Consumer pattern.
     */
    std::queue<std::vector<uint8_t>> data_queue;  
    std::mutex routine_queue_mutex;               
    std::condition_variable wakeUpthread;         
    std::atomic<bool> ProsessThreads_status;      

    /**
     * @brief Logic dispatcher for decrypted packets.
     * Decodes the binary stream and triggers the appropriate EventManager callbacks.
     */
    void HandleReceivedPacket(const std::vector<uint8_t>& data);

    /** @brief Main execution loop for each worker thread in the pool. */
    void RoutineProgressWorkerThread(int threadId);

    /** @brief Enqueues encrypted data to the network layer's egress queue. */
    void SendData_to_Sendque(const std::vector<uint8_t>& data);

    /**
     * @brief Generic pipeline: Serialize -> Encrypt -> Dispatch.
     * @tparam T The packet structure type.
     */
    template<typename T>
    void SerializeAndSendResponse(const T& response_packet);
};

/** Global access to the RoutineProgress singleton. */
extern std::unique_ptr<RoutineProgress> G_Routine;
