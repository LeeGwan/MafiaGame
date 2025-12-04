#ifndef CONNECTSESSIONSERVER_H
#define CONNECTSESSIONSERVER_H

#pragma once
#include <queue>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unordered_map>
#include <vector>

class ClientPacket;
class CompactBinaryReader;

// 세션 서버와의 통신 관리 (로그인 검증용)
class SessionServerConnector {
public:
	SessionServerConnector();
	~SessionServerConnector();
	bool ConnectSession();
	void Release();
	void addToSessionServerQueue(std::vector<u_int8_t>* data);

private:
	std::atomic<bool> ConnectSessionserver_running;
	int SessionSocket;
	std::thread sendwokerthread;
	std::thread Recvwokerthread;

	std::queue<std::vector<u_int8_t>*> sendToSessionServer_queue; // 세션 서버 전송 큐
	std::mutex sendToSessionServer_queue_Mtx;
	std::condition_variable wakeUpSendthread;

	// 워커 스레드
	void SendWoker();
	void RecvWorker();
};

extern std::unique_ptr<SessionServerConnector> G_SessionServerConnector;

#endif