/**
 * @file EventType.h
 * @brief Vocabulary for the centralized EventManager, defining all inter-module communication signals.
 */

#pragma once
#include <cstdint>

/**
 * @enum EventType
 * @brief Strongly typed event identifiers to facilitate decoupled communication between 
 * Network, GUI, Anti-Cheat, and Core Process modules.
 */
enum class EventType : uint8_t
{
    // --- System & Utilities (0x00 - 0x09) ---
    /** Triggers a system message box via ProcessHandler. */
    MESSAGE_EVENT = 0x0,
    /** Signals a fatal SEH exception for recovery or logging. */
    EXCEPTION_ERROR = 0x1,
    /** Gracefully terminates the entire application. */
    TERMINATE_PROCESS_EVENT = 0x2,

    // --- Network Connection Lifecycle (0x0A - 0x14) ---
    /** Immediate transmission of high-priority security/system packets. */
    PRIORITY_PACKET = 0x0A,
    /** Triggered after Routine server response to initiate Auth server connection. */
    SUCCESS_ROUTINEAUTH = 0x0B,
    /** Triggered upon successful account authentication. */
    SUCCESS_AUTH = 0x0C,
    /** Initiates connection to the Game Lobby server post-login. */
    SUCCESS_GAMELOBBY = 0x0D,
    /** Final transition: Connects to the dedicated game server instance. */
    STARTGAME_EVENT = 0x0E,

    // --- Packet Request Dispatchers (0x15 - 0x24) ---
    /** Requests a header-only TypePacket. */
    TYPE_PACKET_REQUEST_EVENT = 0x15,
    /** Requests a TwoStringPacket (e.g., Login or Registration). */
    TWO_STRING_PACKET_EVENT = 0x16,
    /** Requests a HashPacket for session validation. */
    HASH_PACKET_EVENT = 0x17,
    /** Requests a ResultPacket transmission. */
    RESULT_PACKET_EVENT = 0x18,
    /** Initiates the logout sequence and session cleanup. */
    LOGOUT_EVENT = 0x19,

    // --- Game Logic & Matchmaking (0x25 - 0x2F) ---
    /** Validates current session integrity with the Lobby server. */
    CHECK_LOBBY_EVENT = 0x25,
    /** Requests entry into a specific game room. */
    JOIN_ROOM_EVENT = 0x26,
    /** Cancels an active matchmaking or room-entry request. */
    CANCEL_ROOM_EVENT = 0x27,

    // --- GUI & UX Orchestration (0x30 - 0x34) ---
    /** Triggers a UI state transition (e.g., Login -> Lobby). */
    CHANGE_UI_TYPE = 0x30,

    // --- Security & Anti-Cheat (0x35 - 0x3F) ---
    /** Initializes the anti-cheat driver and security hooks. */
    SECURITY_INIT_EVENT = 0x35,
    /** Collects hardware identifiers (HWID) for server-side verification. */
    HWID_DATA_EVENT = 0x36,
    /** Responds to a periodic security heartbeat challenge. */
    SECURITY_HEARTBEAT_EVENT = 0x37,

    /** Sentinel value for array/map pre-allocation. */
    EVENTSIZE = 0x40,
};
