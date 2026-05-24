// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * @enum PacketType
 * @brief Defines the unique identifiers for every binary message in the network protocol.
 * Divided into sections based on server responsibility (Auth, Game, Lobby).
 */
enum class PacketType : uint8_t {
    // --- Auth & Routine Server Protocol ---
    Error = 0,
    RegisterRequest = 1,
    RegisterResponse = 2,
    LoginRequest = 3,
    LoginResponse = 4,
    LogoutRequest = 5,
    LogoutResponse = 6,
    Heartbeat = 7,              // Connection keep-alive
    HeartbeatResult = 8,
    JoinRoomRequest = 9,
    JoinRoomResponse = 10,
    CancelRoomRequest = 11,
    CancelRoomResponse = 12,
    ConnectionCheck = 13,       // Latency and reachability test
    StressCheck = 14,           // Load testing packet

    // --- Game Server Protocol ---
    GameCreate = 30,            // Match initialization
    GameFinish = 31,            // Post-game cleanup
    GameTimeSync = 32,          // Latency compensation & clock sync
    PlayerTransform = 33,       // Position/Rotation updates

    // --- Internal Auth-Routine Protocol ---
    FindAccountServerRequest = 50,
    FindAccountServerResponse = 51,

    // --- Session & Access Control ---
    CheckSessionRequest = 100,  // Session token validation
    CheckSessionResponse = 101,
    CanAccessLobbyRequest = 102, // Permission check for lobby entry
    CanAccessLobbyResponse = 103,

    // --- Game Lobby Server Protocol ---
    TryConnectLobbyServerRequest = 150,
    TryConnectLobbyServerResponse = 151,

    MaxPacketSize = 200         // Protocol boundary for validation
};

/**
 * @enum ResultType
 * @brief Standardized result codes for request-response cycles.
 */
enum class ResultType : uint8_t {
    // Sign-up results
    SignUp_Failed = 0x0,
    SignUp_AlreadyExists,
    SignUp_Succeeded,

    // Authentication results
    Login_Failed,
    Login_InvalidCredentials,
    Login_AlreadyLoggedIn,
    Login_Succeeded,
    Login_InGame,              // Rejection: Account already in an active session

    // Session and Logout results
    CheckSession_Succeeded,
    CheckSession_Failed,
    LogOut_Succeeded,
    LogOut_Failed,

    // Matchmaking and Room results
    JoinRoom_Succeeded,
    JoinRoom_Failed,
    CancelRoom_Succeeded,
    CancelRoom_Failed
};

/**
 * @section Packet_Structures
 * Binary-aligned structures used for serialization/deserialization.
 */

/** Basic packet containing only a type identifier. */
struct TypePacket {
    PacketType Type;
};

/** Generic packet for dual-string payloads (e.g., ID/Password, Token/Value). */
struct TwoStringPacket {
    PacketType Type;
    FString str1;
    FString str2;
};

/** Standard response packet containing an operation result. */
struct ResultPacket {
    PacketType Type;
    ResultType ResultTypes;
};

/** Combined packet for operation result and session/authentication hash. */
struct ResultAndHashPacket {
    PacketType Type;
    ResultType ResultTypes;
    FString hash = "";
};

/** Packet containing only a session or identification hash. */
struct HashPacket {
    PacketType Type;
    FString hash = "";
};

/** Server metadata packet for cross-server redirection or discovery. */
struct ServerInfoPacket {
    PacketType Type;
    FString IP = "";
    uint16_t port;
};

/** Multi-hash authentication data container. */
struct FUserAuthData {
    PacketType Type;
    TArray<FString> hash;
};
