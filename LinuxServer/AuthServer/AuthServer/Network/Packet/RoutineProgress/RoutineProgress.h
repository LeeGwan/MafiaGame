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
#include <vector>

class Aes;
class MafiaDatabase;
class CompactBinaryReader;
enum class ResultType : uint8_t;
enum class PacketType : uint8_t;
struct TypePacket;
struct ResultPacket;
struct ResultAndHashPacket;
struct HashPacket;
struct ServerInfoPacket;
struct ClientPacket;

// 인증 서버의 패킷 처리 엔진 (Thread Pool 기반)
class RoutineProgress {
public:
    RoutineProgress(uint8_t in_threadcount);
    ~RoutineProgress();

    RoutineProgress(const RoutineProgress&) = delete;
    RoutineProgress& operator=(const RoutineProgress&) = delete;

    // 패킷을 처리 큐에 추가
    void addToProgressQueue(ClientPacket* Q_data);
    void Release();

private:
    uint8_t threadcount;
    std::unique_ptr<MafiaDatabase> MafiaDB_server;
    std::vector<std::thread> ProsessThreads;

    std::queue<ClientPacket*> data_queue; // 패킷 처리 대기 큐

    std::mutex routine_queue_mutex;
    std::condition_variable wakeUpthread;
    std::atomic<bool> ProsessThreads_status;

    // 워커 스레드 실행 함수
    void RoutineProgressWorkerThread(int threadId);

    // 패킷 처리 핸들러
    void HandleReceivedPacket(int clientSocket, const std::vector<uint8_t>& data);
    void HandleRegisterPacket(int clientSocket, CompactBinaryReader& reader);
    void HandleLoginPacketWrapper(int clientSocket, CompactBinaryReader& reader);
    void HandleLogoutPacketWrapper(int clientSocket, CompactBinaryReader& reader);

    ResultType HandleLoginPacket(int clientSocket, const std::string& id,
        const std::string& pw, std::string& hash);
    bool HandleLogOutPacket(int clientSocket, const std::string& hash);

    // 패킷 전송
    void SendData_to_Sendque(int clientsock, std::vector<uint8_t>* data);
    void SendResultPacket(int clientsock, PacketType type, ResultType result,
        const std::string& hash = "");
};