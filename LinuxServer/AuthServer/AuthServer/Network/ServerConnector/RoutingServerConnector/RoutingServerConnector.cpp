#include "RoutingServerConnector.h"
#include "../../../MemoryPool/MemoryPool.h"
#include "../../Aes/Aes.h"
#include "../../Packet/CompactBinaryReader/CompactBinaryReader.h"
#include "../../Packet/OptimizedBinaryPacketSerializer/OptimizedBinaryPacketSerializer.h"
#include "../../Packet/PacketStructure/PacketStructure.h"
#include <arpa/inet.h>
#include <functional>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
// 전역 인스턴스 초기화
std::unique_ptr<RoutingServerConnector> G_RoutingServerConnector =
    std::make_unique<RoutingServerConnector>();

RoutingServerConnector::RoutingServerConnector() {}
RoutingServerConnector::~RoutingServerConnector() {Release();}

// 라우팅 서버 연결
bool RoutingServerConnector::ConnectRoutingServer() {
  RoutingServerSocket = socket(AF_INET, SOCK_STREAM, 0);

  struct timeval timeout;
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(8010);
  addr.sin_addr.s_addr = inet_addr("172.30.1.13");

  if (connect(RoutingServerSocket, (sockaddr *)&addr, sizeof(addr)) < 0) {
    close(RoutingServerSocket);
    return false;
  }
  struct sockaddr_in name;
  socklen_t namelen = sizeof(name);

  if (getsockname(RoutingServerSocket, (struct sockaddr *)&name, &namelen) ==
      -1) {
    close(RoutingServerSocket);
    return false;
  }
  // 나의 서버 정보 패킷 전송
  ServerInfoPacket myinfo;
  myinfo.Type=PacketType::ConnectionCheck;
  myinfo.IP = std::string(inet_ntoa(name.sin_addr));
  myinfo.port = htons(9000);
  std::vector<u_int8_t> packet, Encryptpacket;
  OptimizedBinaryPacketSerializer::SerializePacket<ServerInfoPacket>(myinfo,
                                                                     &packet);
  AES->Aes_Encrypt(&packet, &Encryptpacket);
    RoutingServer_running.store(true);
RoutingServerConnectorWorker = std::thread(&RoutingServerConnector::worker, this);
  if (send(RoutingServerSocket, Encryptpacket.data(), Encryptpacket.size(), 0) <
      0)
    return false;

 return true;
}

// 데이터 수신 및 Heartbeat 처리
void RoutingServerConnector::worker() {
  std::vector<u_int8_t> *buffer = nullptr;;
 
  while (RoutingServer_running.load()) {
buffer = G_MemoryPool->acquire<std::vector<u_int8_t>>();

    buffer->resize(BUFFER_SIZE);

    int bytes = recv(RoutingServerSocket, buffer->data(), BUFFER_SIZE, 0);

    if (bytes > 0) {
      buffer->resize(bytes);
      HeartBeat(buffer);
    }
    G_MemoryPool->release<std::vector<u_int8_t>>(buffer);

  }
}
// Heartbeat 패킷 처리
void RoutingServerConnector::HeartBeat(std::vector<uint8_t> *data) {
  try {
    if (!AES)
      return;

    // 메모리 할당 및 복호화
    CompactBinaryReader* reader = nullptr;
    std::vector<uint8_t> *decrypted_data =
        G_MemoryPool->acquire<std::vector<uint8_t>>();
    AES->Aes_Decrypt(data, decrypted_data);
    reader = G_MemoryPool->acquire<CompactBinaryReader>();

    PacketType packet_type;
    // 할당한 메모리 버퍼 자동 반환
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
    case PacketType::Heartbeat:
        // 라우팅 서버로부터 받은 하트비트 패킷을 다시 라우팅 서버로 전송
      send(RoutingServerSocket, data->data(), data->size(),
           0);
      break;
    default:
      break;
    }
  } catch (...) {
    return;
  }
}
// 연결 종료 및 스레드 정리
void RoutingServerConnector::Release() {

  try {

    if (!RoutingServer_running.load())
      return;
    RoutingServer_running.store(false);

    if (RoutingServerSocket != -1) {
      shutdown(RoutingServerSocket, SHUT_RDWR);
      close(RoutingServerSocket);
      RoutingServerSocket = -1;
    }

    if (RoutingServerConnectorWorker.joinable()) {
      RoutingServerConnectorWorker.join();
    }

  } catch (...) {
  }
}