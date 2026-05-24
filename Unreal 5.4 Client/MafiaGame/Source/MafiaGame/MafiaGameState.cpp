// MafiaGameState.cpp
#include "MafiaGameState.h"
#include "Net/UnrealNetwork.h"

/**
 * @brief Constructor: Initializes global game state variables.
 */
AMafiaGameState::AMafiaGameState()
{
    CurrentPhase = EGamePhase::Waiting;
    PhaseTimer = 0.0f;
    DayCount = 0;
    MafiaCount = 0;
    CitizenTeamCount = 0;
    AlivePlayerCount = 0;
    ExecutedPlayerId = TEXT("");
    PlayerStartLocations.Empty();
}

/**
 * @brief Configures properties for network replication using Unreal's DOREPLIFETIME system.
 * Ensures critical game state data is synchronized from the server to all clients.
 */
void AMafiaGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // Sync phase and timer information
    DOREPLIFETIME(AMafiaGameState, CurrentPhase);
    DOREPLIFETIME(AMafiaGameState, PhaseTimer);
    
    // Sync session and game progress data
    DOREPLIFETIME(AMafiaGameState, DayCount);
    DOREPLIFETIME(AMafiaGameState, MafiaCount);
    DOREPLIFETIME(AMafiaGameState, CitizenTeamCount);
    DOREPLIFETIME(AMafiaGameState, AlivePlayerCount);
    DOREPLIFETIME(AMafiaGameState, ExecutedPlayerId);
    
    // Sync identity and positional security data
    DOREPLIFETIME(AMafiaGameState, PlayerHashes);
    DOREPLIFETIME(AMafiaGameState, PlayerStartLocations);
}

/**
 * @brief Updates the current game phase (Authority Only).
 */
void AMafiaGameState::SetCurrentPhase(EGamePhase NewPhase)
{
    if (HasAuthority())
    {
        CurrentPhase = NewPhase;
    }
}

/**
 * @brief Updates the phase countdown timer (Authority Only).
 */
void AMafiaGameState::SetPhaseTimer(float NewTimer)
{
    if (HasAuthority())
    {
        PhaseTimer = NewTimer;
    }
}

/**
 * @brief Increments or sets the current in-game day count (Authority Only).
 */
void AMafiaGameState::SetDayCount(int32 NewDay)
{
    if (HasAuthority())
    {
        DayCount = NewDay;
    }
}

/**
 * @brief Updates the number of active Mafia members (Authority Only).
 */
void AMafiaGameState::SetMafiaCount(int32 Count)
{
    if (HasAuthority())
    {
        MafiaCount = Count;
    }
}

/**
 * @brief Updates the number of active Citizen team members (Authority Only).
 */
void AMafiaGameState::SetCitizenTeamCount(int32 Count)
{
    if (HasAuthority())
    {
        CitizenTeamCount = Count;
    }
}

/**
 * @brief Updates the total count of living players (Authority Only).
 */
void AMafiaGameState::SetAlivePlayerCount(int32 Count)
{
    if (HasAuthority())
    {
        AlivePlayerCount = Count;
    }
}

/**
 * @brief Stores the ID of the player currently targeted for execution (Authority Only).
 */
void AMafiaGameState::SetExecutedPlayerId(const FString& PlayerId)
{
    if (HasAuthority())
    {
        ExecutedPlayerId = PlayerId;
    }
}

/**
 * @brief Sets the full list of validated player session hashes (Authority Only).
 */
void AMafiaGameState::SetPlayerHashes(const TArray<FString>& Hashes)
{
    if (HasAuthority())
    {
        PlayerHashes = Hashes;
    }
}

/**
 * @brief Appends a single player session hash to the registry (Authority Only).
 */
void AMafiaGameState::SetPlayerHash(const FString& Hashes)
{
    if (HasAuthority())
    {
        PlayerHashes.Add(Hashes);
    }
}

/**
 * @brief Updates a session hash at a specific index (Authority Only).
 */
void AMafiaGameState::SetAtPlayerHash(int index, const FString& Hashes)
{
    if (HasAuthority())
    {
        PlayerHashes[index] = Hashes;
    }
}

/**
 * @brief Registers a spawn location tied to a specific session hash (Authority Only).
 */
void AMafiaGameState::AddPlayerStartLocation(const FString& PlayerHash, FVector Location)
{
    if (HasAuthority())
    {
        PlayerStartLocations.Add(FPlayerStartData(PlayerHash, Location));
    }
}

/**
 * @brief [Multicast RPC] Broadcasts player death events to all clients for UI notification.
 */
void AMafiaGameState::MulticastNotifyPlayerDeath_Implementation(const FString& PlayerId, const FString& PlayerName)
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red,
            FString::Printf(TEXT("Player %s has been eliminated"), *PlayerName));
    }

    UE_LOG(LogTemp, Warning, TEXT("Notification: Player %s (%s) eliminated"), *PlayerId, *PlayerName);
}

/**
 * @brief Retrieves the designated spawn location associated with a unique session hash.
 * @return FVector representing the spawn coordinates.
 */
FVector AMafiaGameState::GetPlayerStartLocationByHash(const FString& PlayerHash) const
{
    for (const FPlayerStartData& Data : PlayerStartLocations)
    {
        if (Data.PlayerHash == PlayerHash)
        {
            return Data.Location;
        }
    }
    return FVector::ZeroVector;
}

/**
 * @brief [Multicast RPC] Notifies all clients of a phase transition and synchronizes the timer.
 */
void AMafiaGameState::MulticastPhaseChanged_Implementation(EGamePhase NewPhase, float Duration)
{
    CurrentPhase = NewPhase;
    PhaseTimer = Duration;

    UE_LOG(LogTemp, Warning, TEXT("[GameState] Phase Transition: %d, Duration: %.0f seconds"), (int32)NewPhase, Duration);
}

/**
 * @brief [Multicast RPC] Signals the end of the game and declares the winning faction.
 */
void AMafiaGameState::MulticastGameOver_Implementation(bool bMafiaWin)
{
    UE_LOG(LogTemp, Warning, TEXT("[GameState] Game Over - %s Team Wins!"),
        bMafiaWin ? TEXT("Mafia") : TEXT("Citizen"));
}

/**
 * @brief [Multicast RPC] Distributes the final message of an executed player to everyone.
 */
void AMafiaGameState::MulticastNotifyLastWords_Implementation(const FString& PlayerId, const FString& Words)
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow,
            FString::Printf(TEXT("%s's Last Words: %s"), *PlayerId, *Words));
    }

    UE_LOG(LogTemp, Warning, TEXT("Last Words from %s: %s"), *PlayerId, *Words);
}
