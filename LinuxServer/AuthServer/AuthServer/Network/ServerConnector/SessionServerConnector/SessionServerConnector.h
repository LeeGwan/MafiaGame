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

//SessionServer 네트워크 연결 클래스
class SessionServerConnector{
public:
SessionServerConnector();
~SessionServerConnector();
  bool ConnectSession();    // 세션 서버 연결
  void Release();           // 연결 종료 및 스레드 종료
  void addToSessionServerQueue(int clientsock, const std::string &hash,std::vector<u_int8_t>*data);	// 세션서버에게 보낼 패킷을 큐에 추가
  void addToClientQueue(int clientsock,std::vector<u_int8_t>*data); // 클라이언트에게 보낼 패킷을 큐에 추가

private:
  std::atomic<bool> ConnectSessionserver_running;; // 동작 상태
  int SessionSocket;
  std::thread sendwokerthread;			// 전송 스레드
  std::thread Recvwokerthread;			// 수신 스레드
  std::unordered_map<std::string, int> pendingQueues;	// 해시->클라이언트 소켓 
  std::queue<std::vector<u_int8_t>*> sendToSessionServer_queue;	// 세션에 전송할 큐
  //얘 수만개
  std::mutex pendingQueues_Mtx;
  std::mutex sendToSessionServer_queue_Mtx;
  std::condition_variable wakeUpSendthread;

  void SendWoker();					// 전송 처리
  void RecvWorker();				// 수신 처리
  void ProcessPacket(std::vector<uint8_t>* data);	// 패킷 처리
  void HandleCheckSessionResultPacket(CompactBinaryReader &reader,std::vector<uint8_t>* Originaldata); // 로그인 응답 처리
};
extern std::unique_ptr<SessionServerConnector> G_SessionServerConnector;

#endif