// 패킷 처리 엔진 (Thread Pool 기반, AES 암호화/복호화)
// 네트워크로 수신된 패킷을 비동기로 처리하고, 클라이언트 요청을 서버로 전송
#pragma once
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <stdint.h>
#include <unordered_map>
#include <string>
#include <thread>
#include <vector>


class Aes;                           // AES-256 암호화/복호화 클래스
class MafiaDatabase;                 // 데이터베이스 클래스 (미사용)
class CompactBinaryReader;           // 이진 패킷 역직렬화
enum class ResultType : uint8_t;     // 서버 응답 결과 타입
enum class PacketType : uint8_t;     // 패킷 타입
struct TypePacket;                   // 타입만 포함된 패킷
struct ResultPacket;                 // 결과 코드 패킷
struct ResultAndHashPacket;          // 결과 + 세션 해시 패킷
struct HashPacket;                   // 세션 해시 패킷
struct ServerInfoPacket;             // 서버 IP/Port 정보 패킷
struct ClientPacket;                 // 클라이언트 정보 패킷
struct IntegrityCheckPacket;         // 하드웨어 무결성 검증 패킷
struct stringforVectorPacket;        // 문자열 배열 패킷

class RoutineProgress {
public:
	// Thread Pool 초기화 (in_threadcount: Worker Thread 개수)
	RoutineProgress(uint8_t in_threadcount);
	~RoutineProgress();

	// 복사 생성자 및 대입 연산자 삭제 (싱글톤)
	RoutineProgress(const RoutineProgress&) = delete;
	RoutineProgress& operator=(const RoutineProgress&) = delete;

	// TypePacket 전송 (타입만 포함된 단순 패킷)
	void SendResponseForTypePacket(PacketType type);

	// TwoStringPacket 전송 (로그인/회원가입용, str1: 아이디, str2: 비밀번호)
	void SendResponseForTwoStringPacket(PacketType type, const std::string& str1, const std::string& str2);

	// HashPacket 전송 (세션 해시 포함)
	void SendResponseForHashPacket(PacketType type, const std::string& str1);

	// IntegrityCheckPacket 전송 (하드웨어 ID 정보)
	void SendResponseForIntegrityCheckPacket(const IntegrityCheckPacket& packet);

	// stringforVectorPacket 전송 (문자열 배열)
	void SendResponseForstringforVectorPacket(const stringforVectorPacket& packet);

	// ResultPacket 전송 (작업 결과)
	void SendResponseForstringforResultPacket(const ResultPacket& packet);

	// 우선순위 패킷 (즉시 암호화하여 반환, 송신 큐를 거치지 않음)
	std::vector<uint8_t> SendResponseForpriorityPacket(PacketType type, const std::string& str1);

	// 수신된 패킷을 처리 큐에 추가 (네트워크 스레드에서 호출)
	void addToProgressQueue(const std::vector<uint8_t>& data);

	// Worker Thread 정리 및 종료
	void Release();

private:
	uint8_t threadcount;                     // Worker Thread 개수
	std::mutex client_sockets_mutex;         // 소켓 관리용 뮤텍스 (미사용)
	std::vector<std::thread> ProsessThreads; // Worker Thread 배열

	std::queue<std::vector<uint8_t>> data_queue;  // 처리 대기 중인 패킷 큐
	std::mutex routine_queue_mutex;               // 큐 접근 동기화
	std::condition_variable wakeUpthread;         // Worker Thread 깨우기
	std::atomic<bool> ProsessThreads_status;      // Worker Thread 실행 상태

	// 수신 패킷 AES 복호화 및 타입별 처리 (FindAccountServerResponse, LoginResponse 등)
	void HandleReceivedPacket(const std::vector<uint8_t>& data);

	// Worker Thread 메인 루프 (큐에서 패킷을 꺼내 HandleReceivedPacket 호출)
	void RoutineProgressWorkerThread(int threadId);

	// 네트워크 송신 큐에 데이터 추가
	void SendData_to_Sendque(const std::vector<uint8_t>& data);

	// 패킷 직렬화 + AES 암호화 + 송신 큐 추가 (템플릿)
	template<typename T>
	void SerializeAndSendResponse(const T& response_packet);
};

// 전역 싱글톤 인스턴스
extern std::unique_ptr<RoutineProgress> G_Routine;