/**
 * @file ServerType.h
 * @brief Defines the distinct roles and categories of server instances within the backend infrastructure.
 */

#pragma once
#include <cstdint>

/**
 * @enum ServerType
 * @brief Categorizes the server instances to facilitate service discovery and specialized packet routing.
 * * This enumeration is used by the Load Balancer and Gateway to direct client requests 
 * to the appropriate service layer based on the current application state.
 */
enum class ServerType : uint8_t
{
    /** Default idle state or unassigned server role. */
    WAIT,

    /** Handles routine security validation, heartbeats, and periodic anti-cheat synchronization. */
    ROUTINEAUTHSERVER,

    /** Primary authentication node for login, registration, and session token issuance. */
    AUTHSERVER,

    /** Manages game rooms, matchmaking queues, and pre-game social interactions. */
    GAMELOBBYSERVER
};
