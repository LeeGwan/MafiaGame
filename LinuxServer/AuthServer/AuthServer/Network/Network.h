#pragma once
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <queue>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
class RoutineProgress;
struct ClientPacket;
// Network 클래스: 서버 소켓 관리, 클라이언트 연결/수신/송신 처리
class Network {
public:
  Network();
  ~Network();
  // 서버 초기화 및 외부 세션 연결
  bool ConnectSession();
  // 네트워크 스레드 시작 (recvThreadCount: 수신 스레드, sendThreadCount: 송신 스레드)
  void StartNetwork(unsigned int recvthreadcount,
                           unsigned int sendthreadcount);
  // 특정 소켓에 전송할 패킷을 큐에 추가
  void addToSendQueue(int socket,bool isLast, std::vector<uint8_t> *data);
  // 서버 및 클라이언트 연결 종료, 메모리 해제
  void Release();

private:
  std::unique_ptr<RoutineProgress> Routine;
  const int MAX_EVENTS = 64;        // epoll 이벤트 최대 개수
  const int BUFFER_SIZE = 1024;     // 클라이언트 수신 버퍼 크기
  std::atomic<bool> running;        // 네트워크 동작 상태
  int port;
  int server_fd;                    
  int epoll_fd;
  struct sockaddr_in server_addr;

  std::vector<int> recv_epoll_fd;   // 수신 전용 epoll fd 배열
  std::vector<std::thread> recvThreads;

  std::vector<int> send_epoll_fd;   // 송신 전용 epoll fd 배열
  std::vector<std::thread> sendThreads;
  // 각 소켓별 송신 큐, bool: 마지막 패킷 여부, vector<uint8_t>*: 데이터
  std::unordered_map<int, std::queue<std::pair<bool,std::vector<uint8_t> *>>> send_queues;
  // 송신 큐 동기화
  std::mutex send_queue_mutex;

  // 현재 연결된 클라이언트
  std::unordered_set<int> client_sockets;
  std::mutex client_sockets_mutex;

private:
  bool set_nonblocking(int fd);                         // 소켓을 논블로킹 모드로 설정
  void recvWorkerThread(int threadId);                  // 수신 스레드 함수
  void sendWorkerThread(int threadId);                  // 송신 스레드 함수
  void AcceptConnection(int worker_count);              // 새로운 클라이언트 연결 처리

  void processSendQueue(int fd, int threadId);          // 소켓의 송신 큐 처리
  void removeConnection(int socket, int threadId);      // 연결 종료 및 정리
};
extern std::unique_ptr<Network> G_network;

//