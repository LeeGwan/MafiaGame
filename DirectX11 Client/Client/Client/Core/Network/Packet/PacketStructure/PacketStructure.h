/**
 * @file PacketStructure.h
 * @brief Definitions of network packet headers, result codes, and data structures.
 */

#pragma once
#include <cstdint>
#include <string>
#include <vector>

/**
 * @enum PacketType
 * @brief Unique identifiers for network messages across different server roles.
 */
enum class PacketType : uint8_t {
    // --- General / Error ---
    Error = 0,
    
    // --- Authentication & Account (AuthServer) ---
    RegisterRequest = 1,
    RegisterResponse = 2,
    LoginRequest = 3,
    LoginResponse = 4,
    LogoutRequest = 5,
    LogoutResponse = 6,
    Heartbeat = 7,

    // --- Matchmaking & Room Management (LobbyServer) ---
    JoinRoomRequest = 9,
    JoinRoomResponse = 10,
    CancelRoomRequest = 11,
    CancelRoomResponse = 12,

    // --- Session & Service Discovery (RoutineServer) ---
    GameCreate = 30,
    FindAccountServerRequest = 50,
    FindAccountServerResponse = 51,
    CheckSessionRequest = 100,
    CanAccessLobbyRequest = 102,

    // --- System & Security (Anti-Cheat) ---
    TryConnectLobbyServerRequest = 150,
    TryConnectLobbyServerResponse = 151, 
    ANTI_EVENT_REQUEST = 152,
    ANTI_EVENT_Response = 153,
    HeartbeatRequest = 154,
    HeartbeatResponse = 155,
};

/**
 * @enum ResultType
 * @brief Standardized result codes for request validation and process outcomes.
 */
enum class ResultType : uint8_t {
    // Account Action Results
    SignUp_Failed = 0x0,
    SignUp_AlreadyExists = 0x1,
    SignUp_Succeeded = 0x2,
    SignUp_Not_Match = 0x3,
    
    // Authentication Results
    Login_Failed = 0x4,
    Login_InvalidCredentials = 0x5,
    Login_AlreadyLoggedIn = 0x6,
    Login_Succeeded = 0x7,
    Login_InGame = 0x8,
    Is_Ban = 0x9,
    
    // Session & Connection Results
    CheckSession_Succeeded = 0x10,
    CheckSession_Failed = 0x11,
    LogOut_Succeeded = 0x12,
    LogOut_Failed = 0x13,
    
    // Room & Matchmaking Results
    JoinRoom_Succeeded = 0x14,
    JoinRoom_Failed = 0x15,
    CancelRoom_Succeeded = 0x16,
    CancelRoom_Failed = 0x17,

    // Security / Driver Status Results
    Flect_Not_Running = 0x18,
    Flect_Running = 0x19
};

// --- Specialized Packet Structures ---

/** @brief Minimal header-only packet for simple signaling. */
struct TypePacket {
    PacketType Type;
};

/** @brief Payload containing dual string identifiers (e.g., ID/PW). */
struct TwoStringPacket {
    PacketType Type;
    std::string str1;
    std::string str2;
};

/** @brief Packet for returning a specific operation result. */
struct ResultPacket {
    PacketType Type;
    ResultType ResultTypes;
};

/** @brief Extended result packet including a session validation hash. */
struct ResultAndHashPacket {
    PacketType Type;
    ResultType ResultTypes;
    std::string hash = "";
};

/** @brief Core session identification packet. */
struct HashPacket {
    PacketType Type;
    std::string hash = "";
};

/** @brief Network endpoint distribution packet (Service Discovery). */
struct ServerInfoPacket {
    PacketType Type;
    std::string IP = "";
    uint16_t port;
};

/** @brief Collection-based payload for list synchronization. */
struct stringforVectorPacket {
    PacketType Type;
    std::string hash = "";
    std::vector<std::string> str;
};

/** @brief Hardware-level integrity verification packet for anti-cheat layers. */
struct IntegrityCheckPacket {
    PacketType Type;
    std::string hash = "";
    std::string Mainboard_ID = "";
    std::string CPU_ID;
};
