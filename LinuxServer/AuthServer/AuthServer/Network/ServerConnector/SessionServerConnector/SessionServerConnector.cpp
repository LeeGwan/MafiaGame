#include "SessionServerConnector.h"
#include "../../../MemoryPool/MemoryPool.h"
#include "../../Aes/Aes.h"
#include "../../Network.h"
#include "../../Packet/CompactBinaryReader/CompactBinaryReader.h"
#include "../../Packet/OptimizedBinaryPacketSerializer/OptimizedBinaryPacketSerializer.h"
#include "../../Packet/PacketStructure/PacketStructure.h"
#include <arpa/inet.h>
#include <functional>
#include <mutex>
#include <netinet/in.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <errno.h>
#include <iostream>
std::unique_ptr<SessionServerConnector> G_SessionServerConnector =std::make_unique<SessionServerConnector>();
SessionServerConnector::SessionServerConnector() {}

SessionServerConnector::~SessionServerConnector() {Release();}
// 세션 서버 연결 및 워커 스레드 시작
bool SessionServerConnector::ConnectSession() {
  SessionSocket = socket(AF_INET, SOCK_STREAM, 0);

  struct timeval timeout;
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(8100);
  addr.sin_addr.s_addr = inet_addr("172.30.1.40");
 
  if (connect(SessionSocket, (sockaddr *)&addr, sizeof(addr) )< 0) {
        switch(errno) {
        case ECONNREFUSED:
            std::cout << "서버가 연결을 거부했습니다 (ECONNREFUSED)" << std::endl;
            break;
        case ETIMEDOUT:
            std::cout << "연결 시간이 초과되었습니다 (ETIMEDOUT)" << std::endl;
            break;
        case ENETUNREACH:
            std::cout << "네트워크에 도달할 수 없습니다 (ENETUNREACH)" << std::endl;
            break;
        case EADDRINUSE:
            std::cout << "주소가 이미 사용 중입니다 (EADDRINUSE)" << std::endl;
            break;
        case EINPROGRESS:
            std::cout << "논블로킹 소켓에서 연결 진행 중입니다 (EINPROGRESS)" << std::endl;
            break;
        default:
           break;
    }
    close(SessionSocket);
    return false;
  }

  if (setsockopt(SessionSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                 sizeof(timeout)) < 0) {
                   close(SessionSocket);
    return false;
  }

  ConnectSessionserver_running.store(true);
  sendwokerthread = std::thread(&SessionServerConnector::SendWoker, this);
  Recvwokerthread = std::thread(&SessionServerConnector::RecvWorker, this);

  return true;
}
// 연결 종료 및 스레드 종료
void SessionServerConnector::Release() {
  if(!ConnectSessionserver_running.load())return ;
  ConnectSessionserver_running.store(false);
    
    wakeUpSendthread.notify_all();
    
    if (SessionSocket != -1) {
        shutdown(SessionSocket, SHUT_RDWR);
        close(SessionSocket);
        SessionSocket = -1;
    }
    
    if (sendwokerthread.joinable()) {
        sendwokerthread.join();
    }
    
    if (Recvwokerthread.joinable()) {
        Recvwokerthread.join();
    }
    
    
    {
        std::lock_guard<std::mutex> lock(pendingQueues_Mtx);
        pendingQueues.clear();
    }}

// 세션 서버 전송 큐에 추가
void SessionServerConnector::addToSessionServerQueue(
    int clientsock, const std::string &hash, std::vector<u_int8_t> *data) {
  {
    std::lock_guard<std::mutex> pendinglock(pendingQueues_Mtx);
    //unique ID Hash 값을 Key로 저장
    pendingQueues[hash] = clientsock;
  }
  {
    std::lock_guard<std::mutex> sendToSessionServer_queue_lock(
        sendToSessionServer_queue_Mtx);
    //작업Que를 FIFO 형태로저장
    sendToSessionServer_queue.push(data);
  }
  wakeUpSendthread.notify_one();
}
// 클라이언트 전송 큐에 추가
void SessionServerConnector::addToClientQueue(int clientsock
    ,std::vector<u_int8_t> *data) 
{

  return G_network->addToSendQueue(clientsock,false, data);
}
// (나의서버 -> 세션서버) 전송 워커
void SessionServerConnector::SendWoker() {
  std::vector<u_int8_t> *data = nullptr;

  while (ConnectSessionserver_running.load()) {
    {
      std::unique_lock<std::mutex> lock(sendToSessionServer_queue_Mtx);
      wakeUpSendthread.wait(lock, [&]() {
        return !ConnectSessionserver_running.load() ||
               !sendToSessionServer_queue.empty();
      });

      data = sendToSessionServer_queue.front();
      sendToSessionServer_queue.pop();
    }

    std::unique_ptr<std::vector<u_int8_t>,
                    std::function<void(std::vector<u_int8_t> *)>>
        reguard(data, [](std::vector<u_int8_t> *ptr) {
          G_MemoryPool->release(ptr);
        });
        if(SessionSocket==-1)continue;
    send(SessionSocket, data->data(), data->size(), 0);
  }
}
// (세션서버 -> 나의서버) 수신 워커
void SessionServerConnector::RecvWorker() {
  std::vector<u_int8_t>*buffer;
  while (ConnectSessionserver_running.load()) {
    buffer=G_MemoryPool->acquire<std::vector<u_int8_t>>();
 
        buffer->resize(1024);
    int bytes = recv(SessionSocket, buffer->data(), buffer->size(), 0);
    if (bytes > 0) {
        buffer->resize(bytes);
      ProcessPacket(buffer);
    } else {
      if (errno == ECONNRESET || errno == ENOTCONN) {

        // 여기서 에러처리하자
      }
    }
  }
}
// 수신 패킷 처리
void SessionServerConnector::ProcessPacket(std::vector<uint8_t>* data) {

  try {
    if (!AES)
      return;

    //패킷 메모리 할당 및 복호화
    CompactBinaryReader *reader = nullptr;
    std::vector<uint8_t> *decrypted_data =
    G_MemoryPool->acquire<std::vector<uint8_t>>();
    AES->Aes_Decrypt(data, decrypted_data);
    reader = G_MemoryPool->acquire<CompactBinaryReader>();
    PacketType packet_type;

    //할당된 메모리 자동 반환
    std::unique_ptr<CompactBinaryReader,
                    std::function<void(CompactBinaryReader *)>>
        readerguardforCompactBinaryReader(
            reader, [](CompactBinaryReader *p_reader) {
              G_MemoryPool->release<CompactBinaryReader>(p_reader);
            });
    std::unique_ptr<std::vector<uint8_t>,
                    std::function<void(std::vector<uint8_t> *)>>
        readerguardfordata(decrypted_data, [](std::vector<uint8_t> *p_data) {
          G_MemoryPool->release<std::vector<uint8_t>>(p_data);
        });

    // 패킷 파싱
    if (!OptimizedBinaryPacketSerializer::ParseSecurePacket(
            *decrypted_data, packet_type, reader) ||
        !reader) {
      return;
    }

    switch (packet_type) {
    case PacketType::LoginResponse:
      // 로그인 응답 처리
      HandleCheckSessionResultPacket(*reader,data);
      break;


    default:
        //수신 워커에서 할당된 메모리 자동 반환
          std::unique_ptr<std::vector<u_int8_t>,
                    std::function<void(std::vector<u_int8_t> *)>>
        reguard(data, [](std::vector<u_int8_t> *ptr) {
          G_MemoryPool->release(ptr);
        });
      break;
    }
  } catch (...) {
    return;
  }

}
  // 로그인 응답 처리
  void SessionServerConnector::HandleCheckSessionResultPacket(CompactBinaryReader &reader,std::vector<uint8_t>* Originaldata)
  {
    bool isLast;
      ResultAndHashPacket resultdata;
  OptimizedBinaryPacketSerializer::DeserializePacket<ResultAndHashPacket>(
      reader, resultdata);
      auto it =pendingQueues.find(resultdata.hash);
    if(it!=pendingQueues.end())
    {
      if(resultdata.ResultTypes==ResultType::Login_Succeeded)
      {
        isLast=true;
          std::cout<<'\n'<<"[AuthServer]"<<resultdata.hash<<">>로그인 성공 " <<'\n';

      }
      else
      {
isLast=false;
 std::cout<<'\n'<<"[AuthServer]"<<resultdata.hash<<">>중복 로그인 " <<'\n';
      }
      G_network->addToSendQueue(it->second,isLast,Originaldata);
      
    }
  }