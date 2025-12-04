#pragma once
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <queue>
#include <stdint.h>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

class Aes;
class MafiaDatabase;
class CompactBinaryReader;
class SessionManager;
enum class ResultType : uint8_t;
enum class PacketType : uint8_t;
struct TypePacket;
struct ResultPacket;
struct ResultAndHashPacket;
struct HashPacket;
struct ServerInfoPacket;
struct ClientPacket;

// 게임 로비 서버의 패킷 처리 엔진 (Thread Pool 기반)
class RoutineProgress {
public:
    RoutineProgress(uint8_t in_threadcount);
    ~RoutineProgress();
    RoutineProgress(const RoutineProgress&) = delete;
    RoutineProgress& operator=(const RoutineProgress&) = delete;

    bool CleanupforSocket(int socket);
    int Find_socket(const std::string& hash);
    void addToProgressQueue(ClientPacket* Q_data);
    void Release();
    void Logoutpacket(const std::string& hash);
    void SendPacketToClient(std::vector<uint8_t>* data, int clientSocket,
        bool isLast);
    void SerializeAndSendResponseToDediPacket(int Socket, bool isLast,
        const std::vector<std::string>& hashes);
    void SerializeAndSendResponseForServerInfoPacket(int clientSocket,
        const std::string& IP,
        uint16_t port, bool isLast);

private:
    uint8_t threadcount;
    std::unordered_map<std::string, int> client_sockets; // 세션 토큰 -> 소켓 매핑
    std::mutex client_sockets_mutex;
    std::vector<std::thread> ProsessThreads;
    std::thread HeatbeatThread; // 안티치트 하트비트 스레드

    std::queue<ClientPacket*> data_queue; // 패킷 처리 대기 큐
    std::mutex routine_queue_mutex;
    std::condition_variable wakeUpthread;
    std::atomic<bool> ProsessThreads_status;

    // 패킷 처리
    void HandleReceivedPacket(int clientSocket, const std::vector<uint8_t>& data);

    // 워커 스레드
    void RoutineProgressWorkerThread(int threadId);
    void HeatbeatWoker(int threadId);

    // 유틸리티 함수
    void RequestDisconnect(int clientsock);
    void SendData_to_Sendque(int clientsock, std::vector<uint8_t>* data,
        bool isLast);
    std::vector<uint8_t>* CopyEncryptedData(const std::vector<uint8_t>& source_data);
    void SerializeAndSendResponseForResultPacket(std::vector<uint8_t>* data,
        const ResultPacket& packet,
        int clientSocket, bool isLast);

    template <typename T>
    void SerializePacket(std::vector<uint8_t>* data, const T& response_packet);

    std::vector<uint8_t>* SerializeHashPacketResponse(const HashPacket& response_packet);
};

extern std::unique_ptr<RoutineProgress> G_Routine;