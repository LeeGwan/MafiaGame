/**
 * @file EUIType.h
 * @brief Defines the global UI states for application flow management.
 */

#pragma once
#include <cstdint>

/**
 * @enum EUIType
 * @brief Categorizes the distinct screens/states of the client application.
 * * This enumeration is used by the UI Manager to drive state transitions 
 * and render context-specific interfaces.
 */
enum class EUIType : uint8_t
{
    Init,        // Entry point: Selection between Authentication and Registration
    Register,    // User account creation interface
    Login,       // User authentication interface
    Lobby,       // Game discovery and room selection hub
    Matching,    // Queue management and matchmaking wait state
    Game         // Active game session and gameplay interface
};
