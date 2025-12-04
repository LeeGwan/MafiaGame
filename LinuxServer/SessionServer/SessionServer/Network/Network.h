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

// 세션 관리 서버 네트워크 엔진 (Epoll 기반, 포트 8100)
class Network {
public:
	Network();
	~Network();
	bool Init();
	void StartNetwork(unsigned int recvthreadcount, unsigned int sendthreadcount);
	void addToSendQueue(int socket, std::vector<uint8_t>* data);
	void Release();

private:
	std::unique_ptr<RoutineProgress> Routine;
	const int MAX_EVENTS = 1024;
	const int BUFFER_SIZE = 1024;
	std::atomic<bool> running;
	int port; // 8100 (세션 관리 서버)
	int server_fd;
	int epoll_fd;
	struct sockaddr_in server_addr;

	// 수신 처리
	std::vector<int> recv_epoll_fd;
	std::vector<std::thread> recvThreads;

	// 송신 처리
	std::vector<int> send_epoll_fd;
	std::vector<std::thread> sendThreads;

	std::unordered_map<int, std::queue<std::vector<uint8_t>*>> send_queues; // 송신 큐 (소켓별)
	std::mutex send_queue_mutex;

	std::unordered_set<int> client_sockets; // 연결된 클라이언트 목록
	std::mutex client_sockets_mutex;

private:
	bool set_nonblocking(int fd);
	void recvWorkerThread(int threadId);
	void sendWorkerThread(int threadId);
	void AcceptConnection(int worker_count);
	void processSendQueue(int fd, int threadId);
	void removeConnection(int socket, int threadId);
};

extern std::unique_ptr<Network> G_network;