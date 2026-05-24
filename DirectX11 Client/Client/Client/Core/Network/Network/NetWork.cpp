/**
 * @file NetWork.cpp
 * @brief Implementation of the asynchronous network client using the WSAEventSelect model.
 */

#include "NetWork.h"
#include "../ServerType/ServerType.h"
#include "../Packet/RoutineProgress/RoutineProgress.h"
#include "../Packet/PacketStructure/PacketStructure.h"
#include "../../gui/guicontrol/GuiControl.h"
#include "../../AntiCheat/AntiCheat.h"
#include "../../Core/Core.h"
#include "../../Event/EventManager/EventManager.h"
#include "../../Event/EventType/EventType.h"

/** Global Singleton Instance for Network Orchestration */
std::unique_ptr<NetWork> G_network = std::make_unique<NetWork>();

/**
 * @brief Constructor: Initializes server endpoints and internal states.
 * * Configures default routing and lobby server addresses.
 */
NetWork::NetWork() : currentSocket(INVALID_SOCKET), networkEvent(WSA_INVALID_EVENT), running(false), CurrentState(ServerType::WAIT)
{
    // Internal endpoint configuration
    routineServerIP = "172.30.1.53";
    lobbyServerIP = "172.30.1.60";
    routineServerPort = htons(8000);
    lobbyServerPort = htons(8020);
}

NetWork::~NetWork() { CleanUp(); }

/**
 * @brief Bootstraps WinSock and launches asynchronous processing threads.
 */
bool NetWork::Initialize()
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;

    networkEvent = WSACreateEvent();
    if (networkEvent == WSA_INVALID_EVENT) return false;

    running.store(true);
    
    // Launch dedicated threads for the I/O event loop and the egress (send) queue
    EventThread = std::thread(&NetWork::EventLoop, this);
    sendthread = std::thread(&NetWork::SendLoop, this);

    return true;
}

// --- Connection Dispatchers ---

bool NetWork::ConnectToRoutinAuthServer()
{
    CurrentState.store(ServerType::ROUTINEAUTHSERVER);
    return ConnectToServer(routineServerIP, routineServerPort);
}

bool NetWork::ConnectToAuthServer(const std::string& ip, int port)
{
    CurrentState.store(ServerType::AUTHSERVER);
    return ConnectToServer(ip, port);
}

bool NetWork::ConnectToGameLobbyServer()
{
    std::string hash = G_GuiControl->hash;
    // Security Gate: Ensure anti-cheat is active before lobby entry
    if (hash.empty() || !G_AntiCheat->IsConnected()) return false;

    if (!ConnectToServer(lobbyServerIP, lobbyServerPort)) return false;
    CurrentState.store(ServerType::GAMELOBBYSERVER);
    return true;
}

/**
 * @brief Establishes a TCP connection and registers async events.
 * * Uses WSAEventSelect for non-blocking notification of Connect, Read, and Close events.
 */
bool NetWork::ConnectToServer(const std::string& ip, int port)
{
    currentSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (currentSocket == INVALID_SOCKET) return false;

    // Register WinSock events to the networkEvent object
    if (WSAEventSelect(currentSocket, networkEvent, FD_CONNECT | FD_READ | FD_CLOSE) == SOCKET_ERROR) return false;

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = port;
    serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());

    if (connect(currentSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        if (WSAGetLastError() != WSAEWOULDBLOCK) return false;
    }
    return true;
}

/**
 * @brief Main I/O Event Loop (Worker Thread).
 * * Monitors the WSAEvent object and dispatches notifications to appropriate handlers.
 */
void NetWork::EventLoop()
{
    while (running.load())
    {
        DWORD waitResult = WSAWaitForMultipleEvents(1, &networkEvent, FALSE, 1000, FALSE);
        if (waitResult == WSA_WAIT_EVENT_0)
        {
            WSANETWORKEVENTS networkEvents;
            if (WSAEnumNetworkEvents(currentSocket, networkEvent, &networkEvents) == 0)
            {
                // 1. Connection Established
                if (networkEvents.lNetworkEvents & FD_CONNECT) {
                    if (networkEvents.iErrorCode[FD_CONNECT_BIT] == 0) {
                        if (CurrentState.load() == ServerType::ROUTINEAUTHSERVER)
                            G_Routine->SendResponseForTypePacket(PacketType::FindAccountServerRequest);
                        else if (CurrentState.load() == ServerType::GAMELOBBYSERVER)
                            G_AntiCheat->RequestHardwareInfo(PacketType::TryConnectLobbyServerRequest, G_GuiControl->hash);
                    } else {
                        closesocket(currentSocket);
                        currentSocket = INVALID_SOCKET;
                    }
                }
                // 2. Incoming Data Available
                if (networkEvents.lNetworkEvents & FD_READ) {
                    HandleReceive();
                }
                // 3. Connection Terminated
                if (networkEvents.lNetworkEvents & FD_CLOSE) {
                    if (currentSocket != INVALID_SOCKET) {
                        closesocket(currentSocket);
                        currentSocket = INVALID_SOCKET;
                        WSAEventSelect(currentSocket, networkEvent, 0);
                        WSAResetEvent(networkEvent);
                        // Fallback logic for lobby disconnection
                        if (CurrentState.load() == ServerType::GAMELOBBYSERVER) ConnectToRoutinAuthServer();
                    }
                }
            }
        }
    }
}

/** @brief Ingress: Reads raw bytes from the socket and pushes them to the processing queue. */
void NetWork::HandleReceive()
{
    std::vector<uint8_t> data(BUFFER_SIZE);
    int len;
    do {
        len = recv(currentSocket, reinterpret_cast<char*>(data.data()), (int)data.size(), 0);
        if (len > 0) G_Routine->addToProgressQueue(data);
    } while (len > 0);
}

/** @brief Egress: Enqueues data to the thread-safe send queue. */
void NetWork::addToSendQueue(const std::vector<uint8_t>& data)
{
    {
        std::lock_guard<std::mutex> lock(sendQueue_Mtx);
        sendQueue.push(data);
    }
    wakeSendthread.notify_one();
}

/** @brief Gracefully shuts down all networking threads and releases WinSock resources. */
void NetWork::CleanUp()
{
    if (!running.load()) return;
    running.store(false);

    wakeSendthread.notify_all();

    if (EventThread.joinable()) EventThread.join();
    if (sendthread.joinable()) sendthread.join();

    if (currentSocket != INVALID_SOCKET) closesocket(currentSocket);
    if (networkEvent != WSA_INVALID_EVENT) WSACloseEvent(networkEvent);

    WSACleanup();
}

/**
 * @brief Egress Loop (Worker Thread).
 * * Consumes the send queue and transmits data to the active socket.
 */
void NetWork::SendLoop()
{
    while (running.load())
    {
        std::vector<uint8_t> data;
        {
            std::unique_lock<std::mutex> lock(sendQueue_Mtx);
            wakeSendthread.wait(lock, [this] { return !sendQueue.empty() || !running.load(); });
            if (!running.load()) break;
            
            data = std::move(sendQueue.front());
            sendQueue.pop();
        }
        if (!data.empty() && currentSocket != INVALID_SOCKET)
        {
            send(currentSocket, reinterpret_cast<char*>(data.data()), (int)data.size(), 0);
        }
    }
}

/** @brief Immediate Transmission: Bypasses the queue for critical priority packets. */
void NetWork::priorityPacket(PacketType type, const std::string& str1)
{
    std::vector<uint8_t> data = G_Routine->SendResponseForpriorityPacket(type, str1);
    send(currentSocket, reinterpret_cast<char*>(data.data()), (int)data.size(), 0);
}
