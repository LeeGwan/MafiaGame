#pragma once
#include <cstdint>
#include <string>
#include <vector>
// 패킷 타입
enum class PacketType : uint8_t {
 // RoutineServerorAuthServer ----
  Error = 0,
  RegisterRequest = 1,
  RegisterResponse = 2,
  LoginRequest = 3,
  LoginResponse = 4,
  LogoutRequest = 5,
  LogoutResponse= 6,
  Heartbeat = 7,
  HeartbeatResult = 8,
  JoinRoomRequest = 9,
  JoinRoomResponse =10,
  CancelRoomRequest = 11,
  CancelRoomResponse = 12,
  ConnectionCheck = 13, 
  StressCheck = 14,
  // ---- GameServer ----
  GameCreate = 30,
  GameFinish = 31,
  GameTimeSync = 32,
  PlayerTransform = 33,

  // ---- AuthRoutineServer ----
  FindAccountServerRequest = 50,
  FindAccountServerResponse = 51,

  
  CheckSessionRequest = 100,
  CheckSessionResponse = 101,
  CanAccessLobbyRequest =102,
  CanAccessLobbyResponse  =103,
  //---- GameLobbyServer ----
  TryConnectLobbyServerRequest = 150,
  TryConnectLobbyServerResponse = 151, 
  ANTI_EVENT_REQUEST = 152,
  ANTI_EVENT_Response = 153,
  HeartbeatRequest = 154,
  HeartbeatResponse = 155,
  MaxPacketSize = 200
};
// 결과 타입
enum class ResultType : uint8_t {

 SignUp_Failed = 0x0,
  SignUp_AlreadyExists = 0x1,
  SignUp_Succeeded = 0x2,

  Login_Failed = 0x4,
  Login_InvalidCredentials = 0x5,
  Login_AlreadyLoggedIn = 0x6,
  Login_Succeeded = 0x7,
  Login_InGame = 0x8,
  Is_Ban = 0x9,
  CheckSession_Succeeded = 0x10,
  CheckSession_Failed = 0x11,
  LogOut_Succeeded = 0x12,
  LogOut_Failed = 0x13,
  JoinRoom_Succeeded = 0x14,
  JoinRoom_Failed = 0x15,

  CancelRoom_Succeeded = 0x16,
  CancelRoom_Failed = 0x17,

  Flect_Not_Running = 0x18,
  Flect_Running = 0x19
};
// 다양한 패킷 구조체 정의 (각 구조체는 Type 필드 포함)
struct TypePacket {
  PacketType Type;
};
struct FUserAuthData{
	PacketType Type;
std::vector<std::string>  hash;

};
struct TwoStringPacket {
  PacketType Type;
  std::string str1;
  std::string str2;
};
struct ResultPacket {
  PacketType Type;
  ResultType ResultTypes;
};
struct ResultAndHashPacket {
  PacketType Type;
  ResultType ResultTypes;
  std::string hash = "";
  ;
};
struct HashPacket {
  PacketType Type;
  std::string hash = "";
};
struct ServerInfoPacket {
  PacketType Type;
  std::string IP = "";
  uint16_t port;
};
struct stringforVectorPacket {
	PacketType Type;
	std::string hash = "";
	std::vector<std::string> str;
};
struct IntegrityCheckPacket {
	PacketType Type;
	std::string hash = "";
	std::string Mainboard_ID = "";
	std::string CPU_ID ;
};