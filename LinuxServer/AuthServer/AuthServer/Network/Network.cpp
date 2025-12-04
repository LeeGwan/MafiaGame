#include "Network.h"
#include "../MemoryPool/MemoryPool.h"
#include "Packet/RoutineProgress/RoutineProgress.h"
#include <atomic>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <functional>

#include <netinet/in.h>
#include <queue>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "ServerConnector/RoutingServerConnector/RoutingServerConnector.h"
#include "ServerConnector/SessionServerConnector/SessionServerConnector.h"
#include <stdio.h>

std::unique_ptr<Network> G_network = std::make_unique<Network>();
// 생성자: 서버 포트 및 fd 초기화
Network::Network() : port(9000), server_fd(-1), epoll_fd(-1) {}

// 서버 초기화 및 외부 서버 연결
bool Network::ConnectSession()
{  
  // 패킷 처리용 스레드풀 생성
  Routine = std::make_unique<RoutineProgress>(2);
  struct epoll_event event;
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    return false;
  }
  int opt = 1;
  // SO_REUSEADDR 옵션 설정
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    close(server_fd);
    server_fd = -1;
    return false;
  }
  int serveraddrsize = sizeof(server_addr);
  std::memset(&server_addr, 0x0, serveraddrsize);
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);
  if (bind(server_fd, (struct sockaddr *)&server_addr, serveraddrsize)) {
    close(server_fd);
    server_fd = -1;
    return false;
  }
  if (listen(server_fd, SOMAXCONN)) {
    close(server_fd);
    server_fd = -1;
     return false;
  }
  if (!set_nonblocking(server_fd)) {
    close(server_fd);
    server_fd = -1;
 return false;
  }
  // 외부 서버(Routing, Session) 연결
  if(!G_RoutingServerConnector->ConnectSession()|| !G_SessionServerConnector->ConnectSession())return false;
  // epoll 생성
  epoll_fd = epoll_create(1);
  if (epoll_fd == -1) {
    close(server_fd);
    server_fd = -1;
   return false;
  }
  event.events = EPOLLIN;
  event.data.fd = server_fd;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, event.data.fd, &event) == -1) {
    close(server_fd);
    close(epoll_fd);
    server_fd = -1;
    epoll_fd = -1;
   return false;
  }
  return true;
}
// 소멸자: 네트워크 리소스 해제
Network::~Network() { Release(); }

// 네트워크 스레드 시작
void Network::StartNetwork(unsigned int recvthreadcount,
                           unsigned int sendthreadcount) {
  running.store(true);
  recv_epoll_fd.resize(recvthreadcount);
  send_epoll_fd.resize(sendthreadcount);
  // 송신 큐 미리 예약
  send_queues.reserve(0x10000);

  // 수신 스레드 생성
  for (unsigned int i = 0; i < recvthreadcount; ++i) {
    recv_epoll_fd[i] = epoll_create(1);
    if (recv_epoll_fd[i] == -1) {
      Release();
      return;
    }
    recvThreads.emplace_back(&Network::recvWorkerThread, this, i);
  }
  // 송신 스레드 생성
  for (unsigned int i = 0; i < sendthreadcount; ++i) {
    send_epoll_fd[i] = epoll_create(1);
    if (send_epoll_fd[i] == -1) {
      Release();
      return;
    }
    sendThreads.emplace_back(&Network::sendWorkerThread, this, i);
  }

  // 클라이언트 연결 수락
  AcceptConnection(recvthreadcount);
}
// 네트워크 해제 및 모든 리소스 정리
void Network::Release() {
 if(!running.load())return ;
 G_RoutingServerConnector->Release();
 G_SessionServerConnector->Release();
running.store(false);
  for (auto &thread : recvThreads) {
    if (thread.joinable()) {
      thread.join();
    }
  }
  recvThreads.clear();

  for (auto &thread : sendThreads) {
    if (thread.joinable()) {
      thread.join();
    }
  }
  sendThreads.clear();

  {
    std::lock_guard<std::mutex> lock(client_sockets_mutex);
    for (int client_fd : client_sockets) {
      close(client_fd);
    }
    client_sockets.clear();
  }
  // 송신 큐 비우기
  {
    std::lock_guard<std::mutex> lock(send_queue_mutex);
    for (auto &pair : send_queues) {
      while (!pair.second.empty()) {
        auto data = pair.second.front();
        pair.second.pop();
        G_MemoryPool->release<std::vector<uint8_t>>(data.second);
      }
    }
    send_queues.clear();
  }

  if (Routine) {
    Routine->Release();
    Routine.reset();
  }

  if (server_fd != -1) {
    close(server_fd);
    server_fd = -1;
  }

  if (epoll_fd != -1) {
    close(epoll_fd);
    epoll_fd = -1;
  }

  for (int recv_fd : recv_epoll_fd) {
    if (recv_fd != -1) {
      close(recv_fd);
    }
  }
  recv_epoll_fd.clear();

  for (int send_fd : send_epoll_fd) {
    if (send_fd != -1) {
      close(send_fd);
    }
  }
  send_epoll_fd.clear();
}

// 소켓 논블로킹 설정
bool Network::set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1)
    return false;
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}
// 수신 스레드: epoll 이벤트 기반으로 클라이언트 데이터 수신
void Network::recvWorkerThread(int threadId) {
  struct epoll_event events[MAX_EVENTS];
  ClientPacket *Cpacket = nullptr;
  struct sockaddr_in client_addr;
  socklen_t addr_len = sizeof(client_addr);

  while (running.load()) {
    int eventsize =
        epoll_wait(recv_epoll_fd[threadId], events, MAX_EVENTS, 1000);
    if (eventsize == -1) {
      continue;
    }

    for (int i = 0; i < eventsize; ++i) {
      int fd = events[i].data.fd;
      if (events[i].events & EPOLLIN) {
       //메모리 풀에 ClientPacket메모리 요구
        Cpacket = G_MemoryPool->acquire<ClientPacket>();
        Cpacket->clientSocket = fd;
        Cpacket->data.resize(BUFFER_SIZE);
        ssize_t bytes_received = recv(fd, Cpacket->data.data(), BUFFER_SIZE, 0);
        if (bytes_received > 0) {
          try {
            Cpacket->data.resize(bytes_received);
            //패킷 처리 쓰레드에 작업Que 전달
            Routine->addToProgressQueue(Cpacket);
          } catch (const std::exception &e) {
            G_MemoryPool->release<ClientPacket>(Cpacket);
            removeConnection(fd, threadId);
          }
        }

        else if (bytes_received == 0) {
          // 메모리 반환
          G_MemoryPool->release<ClientPacket>(Cpacket);
          // 연결종료
          removeConnection(fd, threadId);
        } else {
               printf("Socket error: %s (errno: %d)\n", strerror(errno), errno);
          if (errno != EAGAIN && errno != EWOULDBLOCK) {

            G_MemoryPool->release<ClientPacket>(Cpacket);
            removeConnection(fd, threadId);
          }
        }
      }
    }
  }
}
// 송신 스레드: epoll 이벤트 기반으로 클라이언트에 데이터 전송
void Network::sendWorkerThread(int threadId) {
  struct epoll_event events[MAX_EVENTS];
  while (running.load()) {
    //이벤트 발생 할때 까지 대기
    int eventsize =epoll_wait(send_epoll_fd[threadId], events, MAX_EVENTS, 1000);
    if (eventsize == -1) {
      continue;
    }
    //수신
    for (int i = 0; i < eventsize; ++i) {
      int fd = events[i].data.fd;
      if (events[i].events & EPOLLOUT) {
        processSendQueue(fd, threadId);
      }
    }
  }
}
// 클라이언트 연결 수락 및 recv 스레드에 분배
void Network::AcceptConnection(int worker_count) {
  struct epoll_event event;
  struct epoll_event events[MAX_EVENTS];
  struct sockaddr_in client_addr;
  std::atomic<int> next_recv_worker{0};
  socklen_t client_len = sizeof(client_addr);

  while (running.load()) {
    int eventsize = epoll_wait(epoll_fd, events, MAX_EVENTS, 1000);
    if (eventsize == -1) {
      continue;
    }

    for (int i = 0; i < eventsize; ++i) {
      int fd = events[i].data.fd;
      if (fd == server_fd) {
        int clientsock =
            accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (clientsock == -1) {
          continue;
        }

        int recv_worker_index = next_recv_worker.fetch_add(1) % worker_count;

        if (!set_nonblocking(clientsock)) {
          close(clientsock);
          continue;
        }

        event.events = EPOLLIN | EPOLLET;
        event.data.fd = clientsock;
        if (epoll_ctl(recv_epoll_fd[recv_worker_index], EPOLL_CTL_ADD,
                      clientsock, &event) == -1) {
          close(clientsock);
          continue;
        }
        //연결된 클라이언트 소켓 저장
        {
          std::lock_guard<std::mutex> lock(client_sockets_mutex);
          client_sockets.insert(clientsock);
        }
      }
    }
  }
}
// 송신 큐에 데이터 추가 및 epoll 이벤트 등록
void Network::addToSendQueue(int socket,bool isLast, std::vector<uint8_t> *data) {
  {
    //send 쓰레드 작업 Que 전달 
    std::lock_guard<std::mutex> lock(send_queue_mutex);
    send_queues[socket].push({isLast,data});
  }
  //epoll 이벤트 등록
  int send_worker_index = socket % send_epoll_fd.size();
  struct epoll_event event;
  event.events = EPOLLOUT | EPOLLET;
  event.data.fd = socket;

  if (epoll_ctl(send_epoll_fd[send_worker_index], EPOLL_CTL_ADD, socket,
                &event) == -1) {
    epoll_ctl(send_epoll_fd[send_worker_index], EPOLL_CTL_MOD, socket, &event);
  }
}
// 송신 큐 처리
void Network::processSendQueue(int fd, int threadId) {

    // 특정 소켓의 전송 큐를 통째로 가져와 지역 큐로 이동
  std::queue<std::pair<bool,std::vector<uint8_t> *>> local_queue;
  {
    std::lock_guard<std::mutex> lock(send_queue_mutex);
    auto it = send_queues.find(fd);
    if (it == send_queues.end() || it->second.empty()) {
      return;
    }
    local_queue.swap(it->second);
  }
  // 지역 큐에 담긴 패킷들을 순차적으로 송신
  while (!local_queue.empty()) {
    auto data = local_queue.front();
    local_queue.pop();
    // 패킷 메모리 자동 반환을 위한 unique_ptr + 커스텀 deleter
    std::unique_ptr<std::vector<uint8_t>,
                    std::function<void(std::vector<uint8_t> *)>>
        reguard(data.second, [](std::vector<uint8_t> *ptr) {
          G_MemoryPool->release<std::vector<uint8_t>>(ptr);
        });
        int sizes=data.second->size();
    int abc=send(fd, data.second->data(), data.second->size(), MSG_NOSIGNAL);
    if(abc<=0)
    {
        printf("Socket error: %s (errno: %d)\n", strerror(errno), errno);
    }
      // 모든 요청이 끝나면 클라와 연결 종료
      if(data.first)
      {
        removeConnection(fd,threadId);
      }
  }

}
// 연결 종료 및 리소스 해제
void Network::removeConnection(int socket, int threadId) {
    {
    std::lock_guard<std::mutex> lock1(client_sockets_mutex);
    auto it = client_sockets.find(socket);
    if (it == client_sockets.end()) {
      return;
    }
    client_sockets.erase(it);
  }

  {
    std::lock_guard<std::mutex> lock2(send_queue_mutex);
    auto queue_it = send_queues.find(socket);
    if (queue_it != send_queues.end()) {
      while (!queue_it->second.empty()) {
       auto data = queue_it->second.front();
        queue_it->second.pop();
        G_MemoryPool->release<std::vector<uint8_t>>(data.second);
      }
      send_queues.erase(queue_it);
    }
  }

  for (size_t i = 0; i < recv_epoll_fd.size(); ++i) {
    epoll_ctl(recv_epoll_fd[i], EPOLL_CTL_DEL, socket, nullptr);
  }

  for (size_t i = 0; i < send_epoll_fd.size(); ++i) {
    epoll_ctl(send_epoll_fd[i], EPOLL_CTL_DEL, socket, nullptr);
  }

  close(socket);
}