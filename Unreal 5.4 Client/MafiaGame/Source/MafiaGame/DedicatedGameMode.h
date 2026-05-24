// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "ServerConnector.h"
#include "DedicatedGameMode.generated.h"

class AJobs;
class ADirectionalLight;

/**
 * @class ADedicatedGameMode
 * @brief Orchestrates the core game loop, phase transitions, and server-authoritative logic for the Mafia Game.
 * Handles player authentication via external lobby server, job assignment, and network-synchronized environment state.
 */
UCLASS(minimalapi)
class ADedicatedGameMode : public AGameMode
{
    GENERATED_BODY()

protected:
    /** Stores actions taken during the night phase (InstigatorID -> TargetID) */
    TMap<FString, FString> NightActions;

    /** Tracks votes cast during the voting phase (VoterID -> TargetID) */
    TMap<FString, FString> VoteMap;

    /** Temporary storage for authenticated player data during the login handshake (SessionHash -> <Name, IP>) */
    TMap<FString, TPair<FString, FString>> TempPlayerDataMap;

    /** Stores the final message submitted by the condemned player during Last Words phase */
    FString LastWords;

    /** Queue of available spawn locations parsed from the map's PlayerStarts */
    TQueue<FVector> SpawnPosition;

    /** Smart pointer to the custom socket connector for external lobby server communication */
    TUniquePtr<ServerConnector> P_ServerConnector;

protected:
    /** Reference to the primary directional light for Time-of-Day (Day/Night) synchronization */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting", meta = (AllowPrivateAccess = "true"))
    ADirectionalLight* SunLight;

    /** Current cached rotation of the sun to sync with newly joined players */
    UPROPERTY()
    FRotator CurrentSunRotation;

    /** [NetMulticast RPC] Broadcasts sun rotation updates to all connected clients */
    UFUNCTION(NetMulticast, Reliable)
    void MulticastUpdateSunRotation(FRotator NewRotation);

    /** Environmental helpers to toggle visual lighting states */
    void SetDayTime();
    void SetNightTime();

    /** Configuration for game start requirements */
    const int32 REQUIRED_PLAYERS = 6;
    int32 ConnectedPlayers;

    /** Phase Duration Settings (Shortened for testing/development) */
    const float NIGHT_DURATION = 30.0f;
    const float MORNING_DURATION = 30.0f;
    const float VOTING_DURATION = 30.0f;
    const float LASTWORDS_DURATION = 10.0f;

    /** Timer handle for managing scheduled phase transitions */
    FTimerHandle PhaseTimerHandle;

    /** Calculated world location for the execution stand / pedestal */
    FVector ExecutionSiteLocation;

    /** Reference to the authoritative GameState for broadcasting global game data */
    UPROPERTY()
    class AMafiaGameState* MafiaGameState;

public:
    ADedicatedGameMode();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    /**
     * @brief [Auth Override] Validates session hashes with the lobby server before allowing full connection.
     */
    virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

    /**
     * @brief [Auth Override] Finalizes character spawning and player state initialization.
     */
    virtual void PostLogin(APlayerController* NewPlayer) override;

    virtual void RestartPlayer(AController* NewPlayer) override;

    /** Map Parsing & Player Setup */
    void FindPlayerStarts();
    void InitPlayerInformaion(APlayerController* NewPlayer, const FString& UniqueIdStr);

    /**
     * @brief Assigns randomized roles (2 Mafia, 1 Police, 1 Detective, 2 Citizens) to participants.
     */
    UFUNCTION(BlueprintCallable)
    void AssignJobs();

    /** Starts the game session when the required player count is reached */
    UFUNCTION(BlueprintCallable)
    void StartGame();

    /** Game Phase Transition Management */
    void StartNightPhase();
    auto WaitPlayerPhase();
    void StartMorningPhase();
    void StartVotingPhase();
    void StartLastWordsPhase(const FString& PlayerId);
    void ProcessExecution();
    void OnPhaseTimeEnd();

    /** [Server RPC] Handles player interaction during the Night phase (Killing/Investigating) */
    UFUNCTION(Server, Reliable)
    void ServerProcessNightAction(const FString& PlayerId, const FString& TargetId);
    
    /** Resolves the outcome of all night actions simultaneously */
    void ProcessNightResults();

    /** [Server RPC] Processes a vote cast by a player */
    UFUNCTION(Server, Reliable)
    void ServerCastVote(const FString& VoterId, const FString& TargetId);
    
    /** Calculates voting results to determine majority and ties */
    void ProcessVotingResults();

    /** [Server RPC] Submits the last words string from the accused player */
    UFUNCTION(Server, Reliable)
    void ServerSubmitLastWords(const FString& PlayerId, const FString& Words);

    /** Evaluates game state to determine if Mafia or Citizens have achieved victory */
    bool CheckWinCondition();

    /** Helper to find a specific PlayerState using its session hash */
    class AMafiaPlayerState* FindPlayerStateByHash(const FString& PlayerHash);

    /** [NetMulticast RPC] Resets all living players to their assigned pedestals */
    UFUNCTION(NetMulticast, Reliable)
    void MulticastMovePlayersToStart();

    /** [NetMulticast RPC] Toggles movement component functionality for all pawns */
    UFUNCTION(NetMulticast, Reliable)
    void MulticastSetMovementEnabled(bool bEnabled);

    /** Messaging & Feedback Helpers */
    void ALLUpdateMessage(int key, float delay, FColor col, const FString& Text);
    void UpdateMessage(class ADedicatedCharacter* Char, int key, float delay, FColor col, const FString& Text);

    /** [NetMulticast RPC] Notifies clients of a vote count change on a specific player */
    UFUNCTION(NetMulticast, Reliable)
    void MulticastNotifyVoteUpdate(const FString& PlayerId, int32 NewVoteCount);

    /** [Server RPC] Processes and filters chat messages based on game phase and life status */
    UFUNCTION(Server, Reliable)
    void ServerSendChatMessage(const FString& SenderHash, const FString& Message);
};
