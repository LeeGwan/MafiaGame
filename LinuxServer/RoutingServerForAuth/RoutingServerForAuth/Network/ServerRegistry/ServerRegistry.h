#ifndef SERVERREGISTRY_H
#define SERVERREGISTRY_H

#pragma once
#include <atomic>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <queue>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <thread>
#include <unordered_map>
#include <vector>

class ServerInfo;

// 게임 서버 등록 및 관리 시스템 (로드밸런싱)
class ServerRegistry {
public:
	ServerRegistry();
	~ServerRegistry();
	bool Init();
	void Release();
	bool SelectServer(std::string& Ip, uint16_t& port);
	void StartNetwork(unsigned int threadcount);

private:
	const int MAX_EVENTS = 10024;
	const int BUFFER_SIZE = 1024;
	std::atomic<bool> running;
	int port;
	int server_fd;
	int epoll_fd;
	struct sockaddr_in server_addr;

	std::vector<int> epolls_fd;
	std::vector<std::thread> recvThreads;
	std::vector<std::thread> sendThreads;

	std::mutex ServerlistMTX;
	std::unordered_map<int, std::unique_ptr<ServerInfo>> Serverlist; // 등록된 서버 목록
	mutable std::atomic<size_t> currentIndex{ 0 }; // Round-Robin용 인덱스

	bool ConnectSession();
	bool set_nonblocking(int fd);
	void AcceptConnection(int worker_count);
	void recvWorkerThread(int threadId);
	void sendWorkerThread(int threadId);
	void ProcessPacket(int clientsocket, std::vector<uint8_t>* data);
	void removeConnection(int socket);
	void SendHeartbeat(int clientsocket);
};

extern std::unique_ptr<ServerRegistry> G_ServerRegistry;
#endif