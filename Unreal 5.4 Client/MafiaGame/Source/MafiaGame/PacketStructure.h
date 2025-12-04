// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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
	TryConnectLobbyServerRequest =150,
	TryConnectLobbyServerResponse=151,
	MaxPacketSize = 200
  };
// 결과 타입
enum class ResultType : uint8_t {

	SignUp_Failed = 0x0,
	SignUp_AlreadyExists,
	SignUp_Succeeded,

	Login_Failed,
	Login_InvalidCredentials,
	Login_AlreadyLoggedIn,
	Login_Succeeded,
	Login_InGame,

	CheckSession_Succeeded,
	 CheckSession_Failed,
	LogOut_Succeeded,
	LogOut_Failed,
	JoinRoom_Succeeded,
	JoinRoom_Failed,

	 CancelRoom_Succeeded,
	CancelRoom_Failed
  };
// 다양한 패킷 구조체 정의 (각 구조체는 Type 필드 포함)
struct TypePacket {
	PacketType Type;
};
struct TwoStringPacket {
	PacketType Type;
	FString str1;
	FString str2;
};
struct ResultPacket {
	PacketType Type;
	ResultType ResultTypes;
};
struct ResultAndHashPacket {
	PacketType Type;
	ResultType ResultTypes;
	FString hash = "";
};
struct HashPacket {
	PacketType Type;
FString hash = "";
};
struct ServerInfoPacket {
	PacketType Type;
	FString IP = "";
	uint16_t port;
};
struct FUserAuthData{
	PacketType Type;
	TArray<FString> hash;
	
};
