// Fill out your copyright notice in the Description page of Project Settings.

#include "MafiaPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "DedicatedCharacter.h"

/**
 * @brief Constructor: Initializes player-specific game data to default values.
 */
AMafiaPlayerState::AMafiaPlayerState()
{
    PlayerHash = TEXT("");
    JobType = EJobType::None;
    bIsAlive = true;
    VoteCount = 0;
    TargetPlayerId = TEXT("");
    bInvestigationResult = false;
    VisitInfo = TEXT("");
    SpawnPosition = FVector::ZeroVector;
}

/**
 * @brief Registers properties for network replication using Unreal's DOREPLIFETIME.
 * Critical for maintaining synchronized player data across the server and all clients.
 */
void AMafiaPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AMafiaPlayerState, PlayerHash);
    DOREPLIFETIME(AMafiaPlayerState, JobType);
    DOREPLIFETIME_CONDITION(AMafiaPlayerState, bIsAlive, COND_None);
    DOREPLIFETIME(AMafiaPlayerState, VoteCount);
    DOREPLIFETIME(AMafiaPlayerState, TargetPlayerId);
    DOREPLIFETIME(AMafiaPlayerState, bInvestigationResult);
    DOREPLIFETIME(AMafiaPlayerState, VisitInfo);
    DOREPLIFETIME(AMafiaPlayerState, SpawnPosition);
}

/**
 * @brief Returns the string representation of the player's assigned job.
 */
FString AMafiaPlayerState::GetJobName() const
{
    switch (JobType)
    {
    case EJobType::Mafia:     return TEXT("Mafia");
    case EJobType::Police:    return TEXT("Police");
    case EJobType::Detective: return TEXT("Detective");
    case EJobType::Citizen:   return TEXT("Citizen");
    default:                  return TEXT("None");
    }
}

/**
 * @brief Determines if the player's role is eligible for a specialized night-phase action.
 */
bool AMafiaPlayerState::CanNightAction()
{
    switch (JobType)
    {
    case EJobType::Mafia:
    case EJobType::Police:
    case EJobType::Detective: return true;
    default:                  return false;
    }
}

/**
 * @brief Sets the assigned world spawn position (Authority Only).
 */
void AMafiaPlayerState::SetSpawnPosition(const FVector& POS)
{
    if (HasAuthority())
    {
        SpawnPosition = POS;
    }
}

/**
 * @brief Sets the unique session hash for player identification (Authority Only).
 */
void AMafiaPlayerState::SetPlayerHash(const FString& Hash)
{
    if (HasAuthority())
    {
        PlayerHash = Hash;
    }
}

/**
 * @brief Sets the player's display nickname (Authority Only).
 */
void AMafiaPlayerState::SetNickName(const FString& NickName)
{
    if (HasAuthority())
    {
        PlayerNickName = NickName;
    }
}

/**
 * @brief Assigns a job role to the player (Authority Only).
 */
void AMafiaPlayerState::SetJobType(EJobType NewJobType)
{
    if (HasAuthority())
    {
        JobType = NewJobType;
    }
}

/**
 * @brief Updates the player's survival status and refreshes world visibility (Authority Only).
 */
void AMafiaPlayerState::SetAlive(bool bAlive)
{
    if (HasAuthority())
    {
        bIsAlive = bAlive;

        // If the player dies, immediately update visibility for the local client
        if (!bIsAlive)
        {
            UpdateAllCharacterVisibility();
        }
    }
}

/**
 * @brief Explicitly sets the current vote tally (Authority Only).
 */
void AMafiaPlayerState::SetVoteCount(int32 Count)
{
    if (HasAuthority())
    {
        VoteCount = Count;
    }
}

/**
 * @brief Increments the current vote tally by one (Authority Only).
 */
void AMafiaPlayerState::AddVote()
{
    if (HasAuthority())
    {
        VoteCount++;
    }
}

/**
 * @brief Resets the vote tally to zero (Authority Only).
 */
void AMafiaPlayerState::ResetVotes()
{
    if (HasAuthority())
    {
        VoteCount = 0;
    }
}

/**
 * @brief Sets the ID of the target player for night actions (Authority Only).
 */
void AMafiaPlayerState::SetTarget(const FString& TargetId)
{
    if (HasAuthority())
    {
        TargetPlayerId = TargetId;
    }
}

/**
 * @brief Clears the current night action target (Authority Only).
 */
void AMafiaPlayerState::ResetTarget()
{
    if (HasAuthority())
    {
        TargetPlayerId = TEXT("");
    }
}

/**
 * @brief Stores the outcome of a police investigation (Authority Only).
 */
void AMafiaPlayerState::SetInvestigationResult(bool bIsMafia)
{
    if (HasAuthority())
    {
        bInvestigationResult = bIsMafia;
    }
}

/**
 * @brief Records surveillance information for the Detective role (Authority Only).
 */
void AMafiaPlayerState::SetVisitInfo(const FString& Info)
{
    if (HasAuthority())
    {
        VisitInfo = Info;
    }
}

/**
 * @brief Replication notification for the survival status.
 */
void AMafiaPlayerState::OnRep_IsAlive()
{
    UpdateAllCharacterVisibility();
}

/**
 * @brief Core Visibility Logic: Implements "Ghost Mode" visibility rules.
 * Living players cannot see dead players (ActorHiddenInGame = true).
 * Dead players can see all participants (Ghost view).
 */
void AMafiaPlayerState::UpdateAllCharacterVisibility()
{
    UWorld* World = GetWorld();
    if (!World) return;

    APlayerController* MyPC = Cast<APlayerController>(GetOwner());
    if (!MyPC) return;

    AMafiaPlayerState* MyPS = Cast<AMafiaPlayerState>(MyPC->PlayerState);
    if (!MyPS) return;

    bool bIAmAlive = MyPS->GetIsAlive();

    // Iterate through all player controllers in the session
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        if (!PC) continue;

        AMafiaPlayerState* PS = Cast<AMafiaPlayerState>(PC->PlayerState);
        if (!PS) continue;

        APawn* Pawn = PC->GetPawn();
        if (!Pawn) continue;

        // Visibility Rule: 
        // 1. If local player is alive, hide any dead actors to maintain game integrity.
        // 2. If local player is dead, show all actors to allow spectating.
        if (bIAmAlive)
        {
            Pawn->SetActorHiddenInGame(!PS->GetIsAlive());
        }
        else
        {
            Pawn->SetActorHiddenInGame(false);
        }
    }
}

/**
 * @brief [Client RPC] Receives a chat message from the server and broadcasts to UI.
 */
void AMafiaPlayerState::ClientReceiveChatMessage_Implementation(const FString& SenderHash, const FString& SenderName,
    const FString& Message, bool bSenderIsDead, FLinearColor MessageColor)
{
    OnChatMessageReceived.Broadcast(SenderHash, SenderName, Message, bSenderIsDead, MessageColor);
}

/**
 * @brief [Client RPC] Notifies the client of their assigned role at the start of the game.
 */
void AMafiaPlayerState::ClientNotifyJobAssigned_Implementation(EJobType AssignedJob)
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green,
            FString::Printf(TEXT("Role Assigned: %s"), *GetJobName()));
    }
}

/**
 * @brief [Client RPC] Displays the results of the Police investigation to the local client.
 */
void AMafiaPlayerState::ClientNotifyInvestigationResult_Implementation(const FString& TargetId, bool bIsMafia)
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Cyan,
            FString::Printf(TEXT("[Police Report] Subject %s is %s"),
                *TargetId, bIsMafia ? TEXT("MAFIA!") : TEXT("NOT Mafia.")));
    }
}

/**
 * @brief [Client RPC] Displays surveillance info gathered by the Detective to the local client.
 */
void AMafiaPlayerState::ClientNotifyVisitInfo_Implementation(const FString& Info)
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow,
            FString::Printf(TEXT("[Surveillance] %s"), *Info));
    }
}
