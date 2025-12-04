#include "Player.h"
#include "../GameRoom/GameRoom.h"
#include <mutex>

Player::Player() {}

Player::~Player() {}

// 플레이어 초기화
void Player::init_Player() {
	playerNickName.clear();
	status = playerstatus::INIT;
	room = nullptr;
	CountHeartbeat.store(0);
	sendHeartbeat.store(false);
	Last_AnticheatHeartbeat = std::chrono::steady_clock::now();
}

// 닉네임 조회
std::string Player::GetplayerNickName() {
	std::lock_guard<std::mutex> lock(NickName_MTX);
	return playerNickName;
}

// 룸 등록
void Player::Register_Room(GameRoom* in_room) {
	std::lock_guard<std::mutex> lock(room_MTX);
	room = in_room;
}

// 룸 조회
GameRoom* Player::Get_Room() {
	std::lock_guard<std::mutex> lock(room_MTX);
	return room;
}

// 닉네임 설정
void Player::SetplayerNickName(const std::string& in_playerNickname) {
	std::lock_guard<std::mutex> lock(NickName_MTX);
	playerNickName = in_playerNickname;
}

// 상태 조회
playerstatus Player::Getstatus() { return status.load(); }

// 상태 설정
void Player::Setstatus(playerstatus in_status) { status.store(in_status); }

// 소켓 조회
int Player::Get_socket() { return PlayerSocket.load(); }

// 소켓 설정
void Player::Set_socket(int sock) { PlayerSocket.store(sock); }

// 안티치트 하트비트 시간 갱신
void Player::Set_AnticheatHeartbeat(const std::chrono::steady_clock::time_point& in_time) {
	sendHeartbeat.store(false);
	Last_AnticheatHeartbeat = in_time;
}

// 하트비트 전송 상태 확인
bool Player::IsSendHeartbeat() {
	return sendHeartbeat.load();
}

// 하트비트 전송 플래그 설정
void Player::Set_sendHeartbeat(bool in_sendHeartbeat) {
	sendHeartbeat.store(in_sendHeartbeat);
}

// 마지막 하트비트 시간 조회
std::chrono::steady_clock::time_point  Player::Get_AnticheatHeartbeat() {
	return Last_AnticheatHeartbeat;
}

// 벤 카운터 증가
void Player::Ban_stack()
{
	int stack = CountHeartbeat.load();
	stack++;
	CountHeartbeat.store(stack);
}

// 벤 카운터 초기화
void Player::Clear_Ban_stack()
{
	CountHeartbeat.store(0);
}

// 벤 상태 확인 (2회 이상 실패 시)
bool Player::Is_Ban()
{
	return CountHeartbeat.load() >= 2;
}