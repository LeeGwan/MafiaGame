// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"

// 게임 로비 서버와 통신하는 커넥터 (플레이어 인증 정보 수신)
class MAFIAGAME_API ServerConnector
{
public:
	ServerConnector(const FString& InServerIP, int32 InPort);
	~ServerConnector();

	// 서버 연결 중지
	void Stop();

	// 서버 연결 시작 및 인증 정보 수신
	void Start();

private:
	FString ServerIP; // 게임 로비 서버 IP
	int32 ServerPort; // 게임 로비 서버 포트

	ISocketSubsystem* SocketSub;
	FSocket* Socket;
};