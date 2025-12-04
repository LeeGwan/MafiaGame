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
#pragma comment(lib, "ws2_32.lib")
#include <atomic>
#include <condition_variable>
enum class ServerType : uint8_t;
enum class PacketType : uint8_t;
class NetWork
{
public:
	NetWork();
	~NetWork();
	bool Initialize();  // WinSock 초기화, 이벤트 스레드 시작
	bool ConnectToRoutinAuthServer();  // 라우팅 서버 연결 (포트 8000)
	bool ConnectToAuthServer(const std::string& ip, int port);  // 인증 서버 연결
	bool ConnectToGameLobbyServer();  // 게임 로비 서버 연결 (포트 8020)
	void HandleCurrentState();

	void addToSendQueue(const std::vector<uint8_t>& data);  // 송신 큐에 패킷 추가

	void priorityPacket(PacketType type, const std::string& str1);  // 우선순위 패킷 즉시 전송

	void CleanUp();  // 리소스 정리
private:
	bool ConnectToServer(const std::string& ip, int port);  // TCP 소켓 연결
	void EventLoop();  // WSAWaitForMultipleEvents 루프 (FD_CONNECT, FD_READ, FD_CLOSE)
	void SendLoop();  // 송신 스레드 루프

	void HandleReceive();  // 수신 데이터 처리
private:
	std::atomic<ServerType> CurrentState;
	std::atomic<bool> running;
	SOCKET currentSocket;
	WSAEVENT networkEvent;
	const int BUFFER_SIZE = 1024;
	std::string routineServerIP;
	int routineServerPort;
	std::string authServerIP;
	int authServerPort;
	std::string lobbyServerIP;
	int lobbyServerPort;

	std::thread sendthread;
	std::thread EventThread;

	std::queue<std::vector<uint8_t>> sendQueue; // 클라 전송 작업 큐
	std::mutex sendQueue_Mtx;
	std::condition_variable wakeSendthread;

};

extern std::unique_ptr<NetWork> G_network;
