// Fill out your copyright notice in the Description page of Project Settings.

#include "ServerConnector.h"
#include "RoutineProgress.h"

ServerConnector::ServerConnector(const FString& InServerIP, int32 InPort)
	: ServerIP(InServerIP)
	, ServerPort(InPort)
	, Socket(nullptr)
	, SocketSub(nullptr)
{

}

ServerConnector::~ServerConnector()
{
	if (Socket)
	{
		Socket->Close();
		if (SocketSub)
			SocketSub->DestroySocket(Socket);
		Socket = nullptr;
	}
}

// 서버 연결 중지
void ServerConnector::Stop()
{
	if (Socket)
	{
		Socket->Close();
		if (SocketSub)
			SocketSub->DestroySocket(Socket);
		Socket = nullptr;
	}
}

// 게임 로비 서버에 연결 및 플레이어 인증 정보 수신
void ServerConnector::Start()
{
	// 소켓 서브시스템 초기화
	SocketSub = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSub)
	{
		UE_LOG(LogTemp, Error, TEXT("[ServerConnector] SocketSubsystem not found"));
		return;
	}

	// TCP 소켓 생성
	Socket = SocketSub->CreateSocket(NAME_Stream, TEXT("ServerConnectorSocket"), false);
	if (!Socket)
	{
		UE_LOG(LogTemp, Error, TEXT("[ServerConnector] Failed to create socket"));
		return;
	}

	// 서버 주소 설정
	TSharedRef<FInternetAddr> ServerAddr = SocketSub->CreateInternetAddr();
	bool bIsValid = false;
	ServerAddr->SetIp(*ServerIP, bIsValid);
	ServerAddr->SetPort(ServerPort);

	if (!bIsValid)
	{
		UE_LOG(LogTemp, Error, TEXT("[ServerConnector] Invalid IP: %s"), *ServerIP);
		SocketSub->DestroySocket(Socket);
		Socket = nullptr;
		return;
	}

	// 서버 연결
	if (!Socket->Connect(*ServerAddr))
	{
		UE_LOG(LogTemp, Error, TEXT("[ServerConnector] Connection failed"));
		SocketSub->DestroySocket(Socket);
		Socket = nullptr;
		return;
	}

	// 플레이어 인증 정보 수신 (게임 로비 서버로부터)
	TArray<uint8> Buffer;
	Buffer.SetNumUninitialized(1024);
	int32 BytesRead = 0;

	UE_LOG(LogTemp, Error, TEXT("[ServerConnector] Recv Wait"));

	if (Socket && Socket->Recv(Buffer.GetData(), Buffer.Num(), BytesRead))
	{
		if (BytesRead > 0)
		{
			// 수신한 패킷 처리 (플레이어 세션 토큰, 이름 등)
			GRoutineProgress->HandleReceivedPacket(Buffer);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[ServerConnector] Recv failed"));
		}
	}
	return;
}