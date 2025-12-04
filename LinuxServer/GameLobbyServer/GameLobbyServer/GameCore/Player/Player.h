#ifndef PLAYER_H
#define PLAYER_H

#pragma once
#include <atomic>
#include <mutex>
#include <string>
#include <chrono>

// 플레이어 상태
enum class playerstatus : uint8_t {
	INIT = 0,
	INLOBBY = 1,
	INROOM = 2,
	INGAME = 3,
};

class GameRoom;

// 플레이어 정보 관리 클래스
class Player {
public:
	Player();
	~Player();
	void init_Player();

	// 기본 정보
	std::string GetplayerNickName();
	void SetplayerNickName(const std::string& in_playerNickname);

	// 룸 관리
	void Register_Room(GameRoom* in_room);
	GameRoom* Get_Room();

	// 상태 관리
	playerstatus Getstatus();
	void Setstatus(playerstatus in_status);

	// 소켓 관리
	int Get_socket();
	void Set_socket(int sock);

	// 안티치트 하트비트
	void Set_AnticheatHeartbeat(const std::chrono::steady_clock::time_point& in_time);
	std::chrono::steady_clock::time_point  Get_AnticheatHeartbeat();
	bool IsSendHeartbeat();
	void Set_sendHeartbeat(bool in_sendHeartbeat);

	// 벤 시스템
	void Clear_Ban_stack();
	void Ban_stack();
	bool Is_Ban();

private:
	std::string playerNickName;
	std::mutex NickName_MTX;
	std::atomic<playerstatus> status;

	GameRoom* room;
	std::mutex room_MTX;

	std::atomic<int> PlayerSocket;
	std::atomic<uint8_t> CountHeartbeat; // 하트비트 실패 카운트
	std::atomic<bool> sendHeartbeat;
	std::chrono::steady_clock::time_point Last_AnticheatHeartbeat;
};

#endif