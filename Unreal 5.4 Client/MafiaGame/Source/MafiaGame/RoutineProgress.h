// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * @class RoutineProgress
 * @brief Manages the session lifecycle and authorized player registry for the game instance.
 * * This class acts as a localized authentication authority, storing validated session hashes
 * and providing a verification gateway for incoming player connections.
 */
class MAFIAGAME_API RoutineProgress
{
public:
    RoutineProgress();
    ~RoutineProgress();

    /**
     * @brief Processes incoming binary data and updates the internal session registry.
     * @param data Raw byte array received from the authentication or routine server.
     */
    void HandleReceivedPacket(const TArray<uint8_t>& data);

    /** @brief Retrieves the list of currently authorized session hashes. */
    TArray<FString> GetHashes() const;

    /** @brief Retrieves the list of nicknames associated with active sessions. */
    TArray<FString> GetNickNames() const;

    /** @brief Updates the authorized hash whitelist (Authority Only). */
    void SetHashes(const TArray<FString>& In_Hashs);

    /** @brief Updates the associated nicknames list (Authority Only). */
    void SetNickNames(const TArray<FString>& In_NickNames);

    /**
     * @brief Performs a whitelist check to determine if a session hash is authorized to join.
     * @param In_hash The session token provided by the connecting client.
     * @return True if the hash is found in the authorized registry.
     */
    bool CanLogin(const FString& In_hash);

private:
    /** Registry of authorized session tokens synced from the Auth Server. */
    TArray<FString> Hashes;

    /** Registry of player nicknames corresponding to the authorized hashes. */
    TArray<FString> NickNames;
};

/** Global access point for the RoutineProgress manager. */
extern TUniquePtr<RoutineProgress> GRoutineProgress;
