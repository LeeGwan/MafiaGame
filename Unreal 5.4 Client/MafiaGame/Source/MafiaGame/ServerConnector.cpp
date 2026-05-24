// Fill out your copyright notice in the Description page of Project Settings.

#include "ServerConnector.h"
#include "RoutineProgress.h"

/**
 * @brief Constructor: Initializes server identity and socket references.
 * @param InServerIP Target IP address of the Lobby/Auth server.
 * @param InPort Target port of the Lobby/Auth server.
 */
ServerConnector::ServerConnector(const FString& InServerIP, int32 InPort)
    : ServerIP(InServerIP)
    , ServerPort(InPort)
    , Socket(nullptr)
    , SocketSub(nullptr)
{
}

ServerConnector::~ServerConnector()
{
    Stop();
}

/**
 * @brief Safely terminates the socket connection and cleans up the SocketSubsystem resources.
 */
void ServerConnector::Stop()
{
    if (Socket)
    {
        Socket->Close();
        if (SocketSub)
        {
            SocketSub->DestroySocket(Socket);
        }
        Socket = nullptr;
    }
}

/**
 * @brief Establishes a TCP connection to the backend lobby server and retrieves player authorization metadata.
 * This function handles the initial handshake required for populating the dedicated server's session whitelist.
 */
void ServerConnector::Start()
{
    // Initialize the platform-specific Socket Subsystem
    SocketSub = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!SocketSub)
    {
        UE_LOG(LogTemp, Error, TEXT("[ServerConnector] Critical: SocketSubsystem not found."));
        return;
    }

    // Create a TCP (Stream) socket
    Socket = SocketSub->CreateSocket(NAME_Stream, TEXT("ServerConnectorSocket"), false);
    if (!Socket)
    {
        UE_LOG(LogTemp, Error, TEXT("[ServerConnector] Critical: Failed to create TCP socket."));
        return;
    }

    // Configure the remote server address
    TSharedRef<FInternetAddr> ServerAddr = SocketSub->CreateInternetAddr();
    bool bIsValid = false;
    ServerAddr->SetIp(*ServerIP, bIsValid);
    ServerAddr->SetPort(ServerPort);

    if (!bIsValid)
    {
        UE_LOG(LogTemp, Error, TEXT("[ServerConnector] Invalid server IP address: %s"), *ServerIP);
        SocketSub->DestroySocket(Socket);
        Socket = nullptr;
        return;
    }

    // Attempt to establish connection to the backend
    if (!Socket->Connect(*ServerAddr))
    {
        UE_LOG(LogTemp, Error, TEXT("[ServerConnector] Connection to backend server failed at %s:%d"), *ServerIP, ServerPort);
        SocketSub->DestroySocket(Socket);
        Socket = nullptr;
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("[ServerConnector] Successfully connected. Waiting for authorization payload..."));

    /**
     * Blocking Receive: Fetching the initial session data.
     * The Dedicated Server waits for the authorized player list before finalizing its startup routine.
     */
    TArray<uint8> Buffer;
    Buffer.SetNumUninitialized(1024);
    int32 BytesRead = 0;

    if (Socket && Socket->Recv(Buffer.GetData(), Buffer.Num(), BytesRead))
    {
        if (BytesRead > 0)
        {
            UE_LOG(LogTemp, Display, TEXT("[ServerConnector] Received authorization data (%d bytes). Dispatching to RoutineProgress."), BytesRead);
            
            // Dispatch the raw packet to the handler for deserialization and whitelist population
            GRoutineProgress->HandleReceivedPacket(Buffer);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[ServerConnector] Connection closed by remote host or zero bytes received."));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[ServerConnector] Failed to receive authorization packet."));
    }

    return;
}
