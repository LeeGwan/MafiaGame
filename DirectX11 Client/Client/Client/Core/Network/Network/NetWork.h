/**
 * @file NetWork.h
 * @brief Header for the asynchronous network engine using the WinSock2 WSAEventSelect model.
 */

#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <string>
#include <sstream>
#include <functional>
#include <atomic>
#include <condition_variable>

#pragma comment(lib, "ws2_32.lib")

// Forward Declarations for improved compilation speed
enum class ServerType : uint8_t;
enum class PacketType : uint8_t;

/**
 * @class NetWork
 * @brief Orchestrates multi-threaded network communications and server state transitions.
 * * This class manages asynchronous I/O events and maintains a thread-safe send queue.
 * It facilitates a tiered connection architecture: Routine (Discovery) -> Auth (Credentials) -> Lobby (Game Session).
 */
class NetWork
{
public:
    NetWork();
    ~NetWork();

    /** @brief Initializes WinSock 2.2 and bootstraps background I/O threads. */
    bool Initialize();

    /** @brief Establishes a connection to the Routine/Routing server (Default Port: 8000). */
    bool ConnectToRoutinAuthServer();

    /** @brief Establishes a connection to the primary Authentication server. */
    bool ConnectToAuthServer(const std::string& ip, int port);

    /** @brief Establishes a connection to the Game Lobby server (Default Port: 8020). */
    bool ConnectToGameLobbyServer();

    /** @brief Evaluates and transitions the current connection state. */
    void HandleCurrentState();

    /**
     * @brief Enqueues a packet for asynchronous transmission.
     * @param data The serialized and encrypted binary payload.
     */
    void addToSendQueue(const std::vector<uint8_t>& data);

    /**
     * @brief Transmits a high-priority packet immediately, bypassing the send queue.
     * @param type The PacketType identifier.
     * @param str1 The payload string (e.g., session hash).
     */
    void priorityPacket(PacketType type, const std::string& str1);

    /** @brief Gracefully terminates all threads and releases system-level socket resources. */
    void CleanUp();

private:
    /** @brief Internal method to create a TCP socket and register WSA events. */
    bool ConnectToServer(const std::string& ip, int port);

    /** @brief Worker thread loop: Monitors FD_CONNECT, FD_READ, and FD_CLOSE events. */
    void EventLoop();

    /** @brief Worker thread loop: Consumes the send queue and transmits data to the server. */
    void SendLoop();

    /** @brief Ingress handler: Reads raw bytes from the buffer and pushes to the processing engine. */
    void HandleReceive();

private:
    // --- Synchronization & State ---
    std::atomic<ServerType> CurrentState;
    std::atomic<bool> running;
    
    // --- WinSock Handles ---
    SOCKET currentSocket;
    WSAEVENT networkEvent;
    
    // --- Endpoints & Buffers ---
    const int BUFFER_SIZE = 1024;
    std::string routineServerIP;
    int routineServerPort;
    std::string authServerIP;
    int authServerPort;
    std::string lobbyServerIP;
    int lobbyServerPort;

    // --- Threading & Egress Pipeline ---
    std::thread sendthread;
    std::thread EventThread;

    /** @section Send_Pipeline 
     * Implements a Producer-Consumer pattern for non-blocking network transmission.
     */
    std::queue<std::vector<uint8_t>> sendQueue; 
    std::mutex sendQueue_Mtx;
    std::condition_variable wakeSendthread;
};

/** Global access point for the Network singleton instance. */
extern std::unique_ptr<NetWork> G_network;
