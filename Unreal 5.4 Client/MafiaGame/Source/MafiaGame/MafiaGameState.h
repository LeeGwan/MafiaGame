// MafiaGameState.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "MafiaGameState.generated.h"

/**
 * @struct FPlayerStartData
 * @brief Container for mapping player session tokens to their designated world spawn locations.
 */
USTRUCT(BlueprintType)
struct FPlayerStartData
{
    GENERATED_BODY()

    /** Unique session hash used for player identification */
    UPROPERTY()
    FString PlayerHash;

    /** World coordinates for the assigned spawn point */
    UPROPERTY()
    FVector Location;

    FPlayerStartData()
        : PlayerHash(TEXT("")), Location(FVector::ZeroVector)
    {
    }

    FPlayerStartData(FString InHash, FVector InLocation)
        : PlayerHash(InHash), Location(InLocation)
    {
    }
};

/**
 * @enum EGamePhase
 * @brief Defines the distinct stages of a Mafia game session.
 * These phases govern player movement, UI state, and available actions.
 */
UENUM(BlueprintType)
enum class EGamePhase : uint8
{
    Waiting,    // Waiting for all players to authenticate and join
    Night,      // Role-specific action phase (Mafia, Police, Detective)
    Morning,    // Result reveal and general discussion phase
    Voting,     // Majority voting phase to identify the suspect
    LastWords,  // Final speech phase for the accused player
    GameOver    // Session termination and winner declaration
};

/**
 * @class AMafiaGameState
 * @brief Authoritative state manager for the Mafia Game.
 * Synchronizes global game variables, player counts, and phase timers across all clients.
 */
UCLASS()
class MAFIAGAME_API AMafiaGameState : public AGameState
{
    GENERATED_BODY()

protected:
    /** Current active phase of the game session */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
    EGamePhase CurrentPhase;

    /** Remaining time in the current phase */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
    float PhaseTimer;

    /** Progress of the session in days */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
    int32 DayCount;

    /** Total number of Mafia faction members currently alive */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
    int32 MafiaCount;

    /** Total number of Citizen faction members currently alive */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
    int32 CitizenTeamCount;

    /** Sum of all living players across both factions */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
    int32 AlivePlayerCount;

    /** Session ID of the player currently selected for execution */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
    FString ExecutedPlayerId;

    /** Registry of all validated player session hashes for the current match */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
    TArray<FString> PlayerHashes;

    /** Mapping table of session hashes to physical spawn points */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
    TArray<FPlayerStartData> PlayerStartLocations;

public:
    AMafiaGameState();

    /** @brief Required override to register variables for network synchronization */
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** Getters */
    UFUNCTION(BlueprintCallable, Category = "Game State")
    EGamePhase GetCurrentPhase() const { return CurrentPhase; }

    UFUNCTION(BlueprintCallable, Category = "Game State")
    float GetPhaseTimer() const { return PhaseTimer; }

    UFUNCTION(BlueprintCallable, Category = "Game State")
    int32 GetDayCount() const { return DayCount; }

    UFUNCTION(BlueprintCallable, Category = "Game State")
    int32 GetMafiaCount() const { return MafiaCount; }

    UFUNCTION(BlueprintCallable, Category = "Game State")
    int32 GetCitizenTeamCount() const { return CitizenTeamCount; }

    UFUNCTION(BlueprintCallable, Category = "Game State")
    int32 GetAlivePlayerCount() const { return AlivePlayerCount; }

    UFUNCTION(BlueprintCallable, Category = "Game State")
    FString GetExecutedPlayerId() const { return ExecutedPlayerId; }

    UFUNCTION(BlueprintCallable, Category = "Game State")
    TArray<FString> GetPlayerHashes() const { return PlayerHashes; }

    UFUNCTION(BlueprintCallable, Category = "Game State")
    FString GetPlayerHash(int index) const { return PlayerHashes[index]; }

    UFUNCTION(BlueprintCallable, Category = "Game State")
    TArray<FPlayerStartData> GetPlayerStartLocations() const { return PlayerStartLocations; }

    /**
     * @brief Retrieves the assigned spawn location based on a player's session hash.
     */
    UFUNCTION(BlueprintCallable, Category = "Game State")
    FVector GetPlayerStartLocationByHash(const FString& PlayerHash) const;

    /** Setters (Strictly enforced Authority/Server-Only) */
    void SetCurrentPhase(EGamePhase NewPhase);
    void SetPhaseTimer(float NewTimer);
    void SetDayCount(int32 NewDay);
    void SetMafiaCount(int32 Count);
    void SetCitizenTeamCount(int32 Count);
    void SetAlivePlayerCount(int32 Count);
    void SetExecutedPlayerId(const FString& PlayerId);
    void SetPlayerHashes(const TArray<FString>& Hashes);
    void SetPlayerHash(const FString& Hashes);
    void SetAtPlayerHash(int index, const FString& Hashes);
    void AddPlayerStartLocation(const FString& PlayerHash, FVector Location);

    /** [NetMulticast RPC] Notifies clients of a phase transition and synchronized duration. */
    UFUNCTION(NetMulticast, Reliable)
    void MulticastPhaseChanged(EGamePhase NewPhase, float Duration);

    /** [NetMulticast RPC] Signals game conclusion and synchronization of the winning result. */
    UFUNCTION(NetMulticast, Reliable)
    void MulticastGameOver(bool bMafiaWin);

    /** [NetMulticast RPC] Broadcasts a specific player elimination event to all participants. */
    UFUNCTION(NetMulticast, Reliable)
    void MulticastNotifyPlayerDeath(const FString& PlayerId, const FString& PlayerName);

    /** [NetMulticast RPC] Synchronizes and displays the condemned player's final message. */
    UFUNCTION(NetMulticast, Reliable)
    void MulticastNotifyLastWords(const FString& PlayerId, const FString& Words);
};
