// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"

/**
 * @class ServerConnector
 * @brief Establishes a foundational TCP link between the Dedicated Game Server and the Backend Lobby Server.
 * * This class is responsible for the initial "Authoritative Handshake," fetching the validated 
 * player session whitelist required for the game instance to begin its routine.
 */
class MAFIAGAME_API ServerConnector
{
public:
    /**
     * @brief Constructor: Configures the target backend server identity.
     * @param InServerIP The destination IP address of the Lobby/Auth server.
     * @param InPort The destination port for the TCP handshake.
     */
    ServerConnector(const FString& InServerIP, int32 InPort);

    /** @brief Destructor: Ensures active sockets are closed and resources are reclaimed. */
    ~ServerConnector();

    /**
     * @brief Manually terminates the connection and destroys the socket instance.
     * Used for clean shutdowns or error recovery.
     */
    void Stop();

    /**
     * @brief Initializes the Socket Subsystem, connects to the backend, and listens for the authorization payload.
     * This is a critical blocking operation performed during server initialization.
     */
    void Start();

private:
    /** Target IP address for the backend infrastructure. */
    FString ServerIP;

    /** Target port for the backend infrastructure. */
    int32 ServerPort;

    /** Reference to the platform-specific Socket Subsystem (e.g., Windows, Linux). */
    ISocketSubsystem* SocketSub;

    /** The low-level socket handle for raw binary communication. */
    FSocket* Socket;
};
