#ifndef ROUTINGSERVERCONNECTOR_H
#define ROUTINGSERVERCONNECTOR_H
#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <vector>

class CompactBinaryReader;

//RoutingServerForAuth 네트워크 연결 클래스
class RoutingServerConnector {
public:
  RoutingServerConnector();
  ~RoutingServerConnector();  
  void worker();                    // 데이터 수신 및 Heartbeat 처리
  bool ConnectRoutingServer();      // 라우팅 서버 연결
  void Release();                   // 연결 종료 및 스레드 종료

private:
  int RoutingServerSocket;          // 서버 소켓
    const int BUFFER_SIZE = 1024;
  std::atomic<bool> RoutingServer_running;  // 동작 상태
  std::thread RoutingServerConnectorWorker; // 수신 워커 스레드


  void HeartBeat(std::vector<uint8_t>* data);   // Heartbeat 처리
};
// 전역 라우팅 서버 커넥터
extern std::unique_ptr<RoutingServerConnector> G_RoutingServerConnector;

#endif