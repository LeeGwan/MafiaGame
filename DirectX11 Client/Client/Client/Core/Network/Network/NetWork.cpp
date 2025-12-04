// 비동기 네트워크 클라이언트 구현 (WSAEventSelect 기반)
#include "NetWork.h"
#include "../ServerType/ServerType.h"
#include "../Packet/RoutineProgress/RoutineProgress.h"
#include "../Packet/PacketStructure/PacketStructure.h"
#include "../../gui/guicontrol/GuiControl.h"
#include "../../AntiCheat/AntiCheat.h"
#include "../../Core/Core.h"
#include "../../Event/EventManager/EventManager.h"
#include "../../Event/EventType/EventType.h"
std::unique_ptr<NetWork> G_network = std::make_unique<NetWork>();

// 생성자: 라우팅 서버(포트 8000), 게임 로비 서버(포트 8020) IP/포트 설정
NetWork::NetWork() :currentSocket(INVALID_SOCKET), networkEvent(WSA_INVALID_EVENT), running(false), CurrentState(ServerType::WAIT)
{
    routineServerIP = "172.30.1.53";
    lobbyServerIP = "172.30.1.60";
    routineServerPort = htons(8000);
    lobbyServerPort = htons(8020);
}
NetWork::~NetWork()
{
    CleanUp();
}

// WinSock 초기화 및 이벤트/송신 스레드 시작
bool NetWork::Initialize()
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;

    networkEvent = WSACreateEvent();
    if (networkEvent == WSA_INVALID_EVENT)
    {

        return false;
    }

    running.store(true);
    EventThread = std::thread(&NetWork::EventLoop, this);
    sendthread = std::thread(&NetWork::SendLoop, this);

    return true;
}

// 라우팅 서버 연결 (인증 서버 IP 획득용, 포트 8000)
bool NetWork::ConnectToRoutinAuthServer()
{
    CurrentState.store(ServerType::ROUTINEAUTHSERVER);
    return   ConnectToServer(routineServerIP, routineServerPort);;
}

// 인증 서버 연결 (라우팅 서버로부터 받은 IP/포트)
bool NetWork::ConnectToAuthServer(const std::string& ip, int port)
{
    CurrentState.store(ServerType::AUTHSERVER);
    return ConnectToServer(ip, port);

}

// 게임 로비 서버 연결 (포트 8020, 안티치트 연결 확인 후)
bool NetWork::ConnectToGameLobbyServer()
{
    std::string hash = G_GuiControl->hash;
    if (hash.empty())return false;
    if (!G_AntiCheat->IsConnected())return false;
    if (!ConnectToServer(lobbyServerIP, lobbyServerPort))return false;

    CurrentState.store(ServerType::GAMELOBBYSERVER);




}

// TCP 소켓 생성 및 비동기 연결 (WSAEventSelect로 FD_CONNECT, FD_READ, FD_CLOSE 등록)
bool NetWork::ConnectToServer(const std::string& ip, int port)
{
    int error = 0;
    currentSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (currentSocket == INVALID_SOCKET) {
        error = WSAGetLastError();
        return false;
    }

    if (WSAEventSelect(currentSocket, networkEvent,
        FD_CONNECT | FD_READ | FD_CLOSE) == SOCKET_ERROR) {

        return false;
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = port;
    serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());

    if (connect(currentSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        if (WSAGetLastError() != WSAEWOULDBLOCK)
        {

            return false;
        }
    }
    return true;
}

// WSAWaitForMultipleEvents 루프 (FD_CONNECT, FD_READ, FD_CLOSE 이벤트 처리)
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
                if (networkEvents.lNetworkEvents & FD_CONNECT) {
                    if (networkEvents.iErrorCode[FD_CONNECT_BIT] == 0) {
                        if (CurrentState.load() == ServerType::ROUTINEAUTHSERVER)
                        {
                            G_Routine->SendResponseForTypePacket(PacketType::FindAccountServerRequest);
                        }

                        else if (CurrentState.load() == ServerType::GAMELOBBYSERVER)
                        {
                            OutputDebugStringA("성공 \n");
                            std::string hash = G_GuiControl->hash;
                            if (hash.empty())continue;

                            G_AntiCheat->RequestHardwareInfo(PacketType::TryConnectLobbyServerRequest, hash);
                        
                        }

                    }
                    else {
       
                        std::string abc = std::to_string(networkEvents.iErrorCode[FD_CONNECT_BIT]);
                        OutputDebugStringA(abc.c_str());

                        closesocket(currentSocket);
                        currentSocket = INVALID_SOCKET;
                    }
                }
                if (networkEvents.lNetworkEvents & FD_READ) {
                    HandleReceive();
                }
                if (networkEvents.lNetworkEvents & FD_CLOSE)
                {
                    if (currentSocket)
                    {
                        closesocket(currentSocket);
                        currentSocket = INVALID_SOCKET;
                        WSAEventSelect(currentSocket, networkEvent, 0);
                        WSAResetEvent(networkEvent);
                        if (CurrentState.load() == ServerType::GAMELOBBYSERVER)
                        {
                            ConnectToRoutinAuthServer();
                        }
                    }
                }
             
            }
        }
    }
}

// 서버 상태 처리
void NetWork::HandleCurrentState()
{
    switch (CurrentState)
    {
    case::ServerType::WAIT:
    {
        CurrentState.store(ServerType::ROUTINEAUTHSERVER);
        break;
    }
    case::ServerType::ROUTINEAUTHSERVER:
    {
        CurrentState.store(ServerType::ROUTINEAUTHSERVER);
        break;
    }
    case::ServerType::GAMELOBBYSERVER:
    {
        break;
    }

    default:break;
    }
}

// 수신 데이터 처리 (패킷 처리 큐에 추가)
void NetWork::HandleReceive()
{
    std::vector<uint8_t> data;
    data.resize(BUFFER_SIZE);
    int Len;
    do
    {
        Len = recv(currentSocket, reinterpret_cast<char*>(data.data()), data.size(), 0);
        if (Len > 0)
        {
            G_Routine->addToProgressQueue(data);
        }
        else
        {
            break;
        }
    } while (Len > 0);
}

// 송신 큐에 패킷 추가
void NetWork::addToSendQueue(const std::vector<uint8_t>& data)
{
    {
        std::lock_guard<std::mutex> lock(sendQueue_Mtx);
        sendQueue.push(data);
    }
    wakeSendthread.notify_one();
}

// 소켓 및 스레드 정리
void NetWork::CleanUp()
{
    if (!running.load())return;
    running.store(false);


    wakeSendthread.notify_all();

    if (EventThread.joinable()) {
        EventThread.join();
    }

    if (sendthread.joinable()) {
        sendthread.join();
    }


    if (currentSocket != INVALID_SOCKET) {
        closesocket(currentSocket);
        currentSocket = INVALID_SOCKET;
    }


    if (networkEvent != WSA_INVALID_EVENT) {
        WSACloseEvent(networkEvent);
        networkEvent = WSA_INVALID_EVENT;
    }


    WSACleanup();


    {
        std::lock_guard<std::mutex> lock(sendQueue_Mtx);
        while (!sendQueue.empty()) {
            sendQueue.pop();
        }
    }
}

// 송신 큐에서 패킷 가져와 전송
void NetWork::SendLoop()
{
    std::vector<uint8_t> data;
    while (running.load())
    {
        {
            std::unique_lock<std::mutex> lock(sendQueue_Mtx);
            wakeSendthread.wait(lock, [this] {return !sendQueue.empty() || !running.load(); });
            if (!running.load()) {
                break;
            }
            data = std::move(sendQueue.front());
            sendQueue.pop();
        }
        if (!data.empty() && currentSocket != INVALID_SOCKET)
        {
            int result = send(currentSocket, reinterpret_cast<char*>(data.data()),
                static_cast<int>(data.size()), 0);
            data.clear();
        }
    }
}

// 우선순위 패킷 즉시 전송
void NetWork::priorityPacket(PacketType type, const std::string& str1)
{
    std::vector<uint8_t> data = G_Routine->SendResponseForpriorityPacket(type, str1);
    send(currentSocket, reinterpret_cast<char*>(data.data()),
        static_cast<int>(data.size()), 0);

}