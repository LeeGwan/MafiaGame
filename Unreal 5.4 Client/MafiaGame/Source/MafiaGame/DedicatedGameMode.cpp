// Copyright Epic Games, Inc. All Rights Reserved.
// Implementation of the Dedicated Server GameMode for the Mafia Game

#include "DedicatedGameMode.h"
#include "MafiaGameState.h"
#include "MafiaPlayerState.h"
#include "Online/CoreOnline.h" 
#include "RoutineProgress.h"
#include "DedicatedCharacter.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"
#include "Engine/DirectionalLight.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

/**
 * @brief Synchronizes sun rotation across all connected clients.
 * @param NewRotation The target rotation for the directional light.
 */
void ADedicatedGameMode::MulticastUpdateSunRotation_Implementation(FRotator NewRotation)
{
    if (SunLight && SunLight->GetLightComponent())
    {
        SunLight->SetActorRotation(NewRotation);
    }
}

/**
 * @brief Sets the environment to Daytime (Sun angle at -60 degrees).
 */
void ADedicatedGameMode::SetDayTime()
{
    if (!SunLight) return;

    CurrentSunRotation = FRotator(-60.0f, 0.0f, 0.0f);
    MulticastUpdateSunRotation(CurrentSunRotation);
}

/**
 * @brief Sets the environment to Nighttime (Sun angle at -130 degrees).
 */
void ADedicatedGameMode::SetNightTime()
{
    if (!SunLight) return;

    CurrentSunRotation = FRotator(-130.0f, 0.0f, 0.0f);
    MulticastUpdateSunRotation(CurrentSunRotation);
}

/**
 * @brief Constructor: Initializes game state classes and external server connector.
 */
ADedicatedGameMode::ADedicatedGameMode()
{
    ConnectedPlayers = 0;
    LastWords = TEXT("");
    CurrentSunRotation = FRotator(-60.0f, 0.0f, 0.0f);
    PrimaryActorTick.bCanEverTick = true;

    // Set default classes for the Game Session
    GameStateClass = AMafiaGameState::StaticClass();
    PlayerStateClass = AMafiaPlayerState::StaticClass();
    
    // Custom connector for external Lobby/Authentication server
    P_ServerConnector = MakeUnique<ServerConnector>(TEXT("172.30.1.38"), 9050);
}

/**
 * @brief Entry point for the GameMode. Initializes environment and server connection.
 */
void ADedicatedGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        MafiaGameState = Cast<AMafiaGameState>(GameState);
        FindPlayerStarts();

        // Identify the Directional Light actor for Time-of-Day synchronization
        TArray<AActor*> AllActors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), AllActors);
        for (AActor* Actor : AllActors)
        {
            if (Actor->FindComponentByClass<UDirectionalLightComponent>() && 
                Actor->GetName().Contains(TEXT("BP_Directional_Light")))
            {
                SunLight = Cast<ADirectionalLight>(Actor);
                if (SunLight) SetDayTime();
                break;
            }
        }

        // Establish connection to the backend lobby server
        P_ServerConnector->Start();
    }
}

/**
 * @brief Frame update: Handles the phase timer decrement logic.
 */
void ADedicatedGameMode::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (HasAuthority() && MafiaGameState)
    {
        EGamePhase CurrentPhase = MafiaGameState->GetCurrentPhase();
        if (CurrentPhase != EGamePhase::Waiting && CurrentPhase != EGamePhase::GameOver)
        {
            float NewTimer = MafiaGameState->GetPhaseTimer() - DeltaTime;
            MafiaGameState->SetPhaseTimer(NewTimer);
        }
    }
}

/**
 * @brief Validates player credentials (Hash/Nickname) before allowing connection.
 */
void ADedicatedGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
    Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
    
    FString Hash = UGameplayStatics::ParseOption(Options, TEXT("Hash"));
    FString NickName = UGameplayStatics::ParseOption(Options, TEXT("NickName"));

    // Reject connections without a valid session hash
    if (Hash.IsEmpty()) return;

    // Cache player data temporarily for PostLogin initialization
    TempPlayerDataMap.Add(UniqueId.ToString(), TPair<FString, FString>(Hash, NickName));
}

/**
 * @brief Finalizes player setup and triggers game start if all players are connected.
 */
void ADedicatedGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    
    if (HasAuthority())
    {
        FString UniqueIdStr = NewPlayer->GetPlayerState<APlayerState>()->GetUniqueId().ToString();
        InitPlayerInformaion(NewPlayer, UniqueIdStr);

        // Sync environmental state to the newly joined player
        if (SunLight) MulticastUpdateSunRotation(CurrentSunRotation);

        // Auto-start game logic when required player count is met
        if (ConnectedPlayers == REQUIRED_PLAYERS && MafiaGameState->GetCurrentPhase() == EGamePhase::Waiting)
        {
            AssignJobs();
            StartGame();
        }
    }
}

/**
 * @brief Synchronizes PlayerId to the character pawn upon respawn.
 */
void ADedicatedGameMode::RestartPlayer(AController* NewPlayer)
{
    Super::RestartPlayer(NewPlayer);

    if (HasAuthority())
    {
        ADedicatedCharacter* MafiaChar = Cast<ADedicatedCharacter>(NewPlayer->GetPawn());
        AMafiaPlayerState* MafiaPS = Cast<AMafiaPlayerState>(NewPlayer->PlayerState);

        if (MafiaChar && MafiaPS)
        {
            MafiaChar->SetPlayerId(MafiaPS->GetPlayerHash());
        }
    }
}

/**
 * @brief Scans the level for APlayerStart actors to initialize spawn queues and execution sites.
 */
void ADedicatedGameMode::FindPlayerStarts()
{
    TArray<AActor*> FoundStarts;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), FoundStarts);

    if (FoundStarts.Num() < 6) return;

    for (AActor* StartActor : FoundStarts)
    {
        // Designate a specific PlayerStart as the execution podium
        if (StartActor->GetName().Contains(TEXT("PlayerSpwan1_C_17")))
        {
            ExecutionSiteLocation = StartActor->GetActorLocation();
        }
        else
        {
            SpawnPosition.Enqueue(StartActor->GetActorLocation());
        }
    }
}

/**
 * @brief Maps authenticated session data to the Unreal PlayerState and character pawn.
 */
void ADedicatedGameMode::InitPlayerInformaion(APlayerController* NewPlayer, const FString& UniqueIdStr)
{
    if (!HasAuthority() || !MafiaGameState || UniqueIdStr.IsEmpty()) return;

    FVector SpwanPOS;
    AMafiaPlayerState* PS = Cast<AMafiaPlayerState>(NewPlayer->PlayerState);
    ADedicatedCharacter* Char = Cast<ADedicatedCharacter>(NewPlayer->GetPawn());

    if (PS && Char && ConnectedPlayers < REQUIRED_PLAYERS)
    {
        if (SpawnPosition.IsEmpty()) return;

        if (TempPlayerDataMap.Contains(UniqueIdStr))
        {
            TPair<FString, FString> PlayerData = TempPlayerDataMap[UniqueIdStr];
            SpawnPosition.Dequeue(SpwanPOS);
            
            // Assign session-level identity to the player state
            PS->SetPlayerHash(PlayerData.Key);
            PS->SetNickName(PlayerData.Value);
            Char->SetPlayerId(PlayerData.Key);
            Char->SetPlayerName(PlayerData.Value);
            PS->SetSpawnPosition(SpwanPOS);
            
            // Move character to assigned spawn point
            Char->ServerMoveToLocation_Implementation(SpwanPOS);
            MafiaGameState->SetPlayerHash(PlayerData.Key);
            
            TempPlayerDataMap.Remove(UniqueIdStr);
            ConnectedPlayers++;
        }
    }
}

/**
 * @brief Randomly assigns jobs (Mafia, Police, Detective, Citizen) to connected players.
 */
void ADedicatedGameMode::AssignJobs()
{
    if (!HasAuthority() || !MafiaGameState) return;

    TArray<FString> PlayerHashes = MafiaGameState->GetPlayerHashes();
    if (PlayerHashes.Num() != 6) return;

    // Shuffle indices for fair job distribution
    TArray<int32> Indices = { 0, 1, 2, 3, 4, 5 };
    for (int32 i = Indices.Num() - 1; i > 0; --i)
    {
        Indices.Swap(i, FMath::RandRange(0, i));
    }

    TArray<EJobType> Jobs = { EJobType::Mafia, EJobType::Police, EJobType::Detective, 
                              EJobType::Citizen, EJobType::Citizen, EJobType::Citizen };

    int32 MafiaCount = 0;
    int32 CitizenTeamCount = 0;

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        AMafiaPlayerState* PS = Cast<AMafiaPlayerState>(It->Get()->PlayerState);
        ADedicatedCharacter* Char = Cast<ADedicatedCharacter>(It->Get()->GetPawn());
        
        if (PS && Char)
        {
            int32 HashIndex = PlayerHashes.IndexOfByKey(PS->GetPlayerHash());
            if (HashIndex != INDEX_NONE)
            {
                EJobType AssignedJob = Jobs[Indices[HashIndex]];
                PS->SetJobType(AssignedJob);
                Char->UpdateNameplateWidgetForJobs(PS->GetJobName());
                PS->ClientNotifyJobAssigned(AssignedJob);

                if (AssignedJob == EJobType::Mafia) MafiaCount++;
                else CitizenTeamCount++;
            }
        }
    }

    MafiaGameState->SetMafiaCount(MafiaCount);
    MafiaGameState->SetCitizenTeamCount(CitizenTeamCount);
    MafiaGameState->SetAlivePlayerCount(6);
}

/**
 * @brief [Multicast] Resets all living players to their original spawn positions.
 */
void ADedicatedGameMode::MulticastMovePlayersToStart_Implementation()
{
    if (!MafiaGameState) return;

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        AMafiaPlayerState* PS = Cast<AMafiaPlayerState>(It->Get()->PlayerState);
        ADedicatedCharacter* Char = Cast<ADedicatedCharacter>(It->Get()->GetPawn());

        if (PS && Char && PS->GetIsAlive() && !PS->GetSpawnPosition().IsZero())
        {
            Char->ForceMoveToLocation(PS->GetSpawnPosition());
        }
    }
}

void ADedicatedGameMode::StartGame()
{
    if (!HasAuthority() || !MafiaGameState) return;
    MafiaGameState->SetDayCount(1);
    StartMorningPhase();
}

/**
 * @brief Transitions game to the Night phase. Disables movement and enables job abilities.
 */
void ADedicatedGameMode::StartNightPhase()
{
    if (!HasAuthority() || !MafiaGameState) return;

    SetNightTime();
    MafiaGameState->SetCurrentPhase(EGamePhase::Night);
    MafiaGameState->SetPhaseTimer(NIGHT_DURATION);

    MulticastSetMovementEnabled(false);
    MafiaGameState->MulticastPhaseChanged(EGamePhase::Night, NIGHT_DURATION);

    GetWorld()->GetTimerManager().SetTimer(PhaseTimerHandle, this, &ADedicatedGameMode::OnPhaseTimeEnd, NIGHT_DURATION, false);
    ALLUpdateMessage(-1, 20.0f, FColor::Green, TEXT("===== Night Phase Started (Movement Disabled) ====="));
}

void ADedicatedGameMode::WaitPlayerPhase()
{
    if (!HasAuthority() || !MafiaGameState) return;
    MafiaGameState->SetCurrentPhase(EGamePhase::Waiting);
    MulticastSetMovementEnabled(true);
}

/**
 * @brief Transitions game to the Morning phase. Processes results of the previous night.
 */
void ADedicatedGameMode::StartMorningPhase()
{
    if (!HasAuthority() || !MafiaGameState) return;

    SetDayTime();
    if (MafiaGameState->GetDayCount() != 1) ProcessNightResults();
    if (CheckWinCondition()) return;

    MafiaGameState->SetCurrentPhase(EGamePhase::Morning);
    MafiaGameState->SetPhaseTimer(MORNING_DURATION);

    MulticastSetMovementEnabled(true);
    MafiaGameState->MulticastPhaseChanged(EGamePhase::Morning, MORNING_DURATION);

    GetWorld()->GetTimerManager().SetTimer(PhaseTimerHandle, this, &ADedicatedGameMode::OnPhaseTimeEnd, MORNING_DURATION, false);
    ALLUpdateMessage(-1, 20.0f, FColor::Green, TEXT("===== Morning Phase Started (Discussion Enabled) ====="));
}

/**
 * @brief Transitions game to the Voting phase. Teleports players to pedestals.
 */
void ADedicatedGameMode::StartVotingPhase()
{
    if (!HasAuthority() || !MafiaGameState) return;

    SetDayTime();
    MafiaGameState->SetCurrentPhase(EGamePhase::Voting);
    MafiaGameState->SetPhaseTimer(VOTING_DURATION);
    VoteMap.Empty();

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (AMafiaPlayerState* PS = Cast<AMafiaPlayerState>(It->Get()->PlayerState))
        {
            PS->ResetVotes();
        }
    }

    MulticastSetMovementEnabled(false);
    MulticastMovePlayersToStart();

    MafiaGameState->MulticastPhaseChanged(EGamePhase::Voting, VOTING_DURATION);

    GetWorld()->GetTimerManager().SetTimer(PhaseTimerHandle, this, &ADedicatedGameMode::OnPhaseTimeEnd, VOTING_DURATION, false);
    ALLUpdateMessage(-1, 20.0f, FColor::Green, TEXT("===== Voting Phase Started ====="));
}

/**
 * @brief Initiates the Last Words phase for the most-voted player.
 */
void ADedicatedGameMode::StartLastWordsPhase(const FString& PlayerId)
{
    if (!HasAuthority() || !MafiaGameState) return;

    SetDayTime();
    MafiaGameState->SetCurrentPhase(EGamePhase::LastWords);
    MafiaGameState->SetPhaseTimer(LASTWORDS_DURATION);
    MafiaGameState->SetExecutedPlayerId(PlayerId);
    LastWords = TEXT("");

    MafiaGameState->MulticastPhaseChanged(EGamePhase::LastWords, LASTWORDS_DURATION);

    // Teleport the accused to the execution site
    AMafiaPlayerState* ExecutedPS = FindPlayerStateByHash(PlayerId);
    if (ExecutedPS)
    {
        APlayerController* PC = Cast<APlayerController>(ExecutedPS->GetOwner());
        ADedicatedCharacter* Char = PC ? Cast<ADedicatedCharacter>(PC->GetPawn()) : nullptr;
        
        if (Char && !ExecutionSiteLocation.IsZero())
        {
            Char->ForceMoveToLocation(ExecutionSiteLocation);
            ALLUpdateMessage(-1, 20.0f, FColor::Green, FString::Printf(TEXT("%s has been taken to the execution stand."), *ExecutedPS->GetNickName()));
        }
    }

    GetWorld()->GetTimerManager().SetTimer(PhaseTimerHandle, this, &ADedicatedGameMode::OnPhaseTimeEnd, LASTWORDS_DURATION, false);
}

/**
 * @brief Executes the player and updates game state (counts and win conditions).
 */
void ADedicatedGameMode::ProcessExecution()
{
    if (!HasAuthority() || !MafiaGameState) return;

    FString ExecutedPlayerId = MafiaGameState->GetExecutedPlayerId();
    AMafiaPlayerState* ExecutedPS = FindPlayerStateByHash(ExecutedPlayerId);
    if (!ExecutedPS) return;

    ExecutedPS->SetAlive(false);
    ALLUpdateMessage(-1, 20.0f, FColor::Green, FString::Printf(TEXT("%s has been executed. Job: %s"), *ExecutedPS->GetNickName(), *ExecutedPS->GetJobName()));

    MafiaGameState->MulticastNotifyPlayerDeath(ExecutedPlayerId, ExecutedPS->GetNickName());
    MafiaGameState->MulticastNotifyLastWords(ExecutedPS->GetNickName(), LastWords);

    // Refresh visibility for all players based on death status
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (AMafiaPlayerState* PS = Cast<AMafiaPlayerState>(It->Get()->PlayerState))
        {
            PS->UpdateAllCharacterVisibility();
        }
    }

    if (ExecutedPS->GetJobType() == EJobType::Mafia) MafiaGameState->SetMafiaCount(MafiaGameState->GetMafiaCount() - 1);
    else MafiaGameState->SetCitizenTeamCount(MafiaGameState->GetCitizenTeamCount() - 1);

    MafiaGameState->SetAlivePlayerCount(MafiaGameState->GetAlivePlayerCount() - 1);
    LastWords = TEXT("");

    if (CheckWinCondition()) return;

    MafiaGameState->SetDayCount(MafiaGameState->GetDayCount() + 1);
    StartNightPhase();
}

/**
 * @brief Timer callback for phase transitions.
 */
void ADedicatedGameMode::OnPhaseTimeEnd()
{
    if (!HasAuthority() || !MafiaGameState) return;

    switch (MafiaGameState->GetCurrentPhase())
    {
    case EGamePhase::Night:     StartMorningPhase(); break;
    case EGamePhase::Morning:   StartVotingPhase(); break;
    case EGamePhase::Voting:    ProcessVotingResults(); break;
    case EGamePhase::LastWords: ProcessExecution(); break;
    default: break;
    }
}

/**
 * @brief [Server RPC] Processes specific night-time targets selected by players.
 */
void ADedicatedGameMode::ServerProcessNightAction_Implementation(const FString& PlayerId, const FString& TargetId)
{
    if (!HasAuthority() || !MafiaGameState || MafiaGameState->GetCurrentPhase() != EGamePhase::Night) return;

    AMafiaPlayerState* PS = FindPlayerStateByHash(PlayerId);
    if (!PS || !PS->GetIsAlive() || !PS->CanNightAction()) return;

    AMafiaPlayerState* TPS = FindPlayerStateByHash(TargetId);
    
    NightActions.Add(PlayerId, TargetId);
    PS->SetTarget(TargetId);

    if (TPS && TPS->GetIsAlive())
    {
        APlayerController* PC = Cast<APlayerController>(PS->GetOwner());
        ADedicatedCharacter* Char = PC ? Cast<ADedicatedCharacter>(PC->GetPawn()) : nullptr;
        UpdateMessage(Char, -1, 20.0f, FColor::Green, FString::Printf(TEXT("Selected target: %s"), *TPS->GetNickName()));
    }
}

/**
 * @brief Calculates and broadcasts the results of night-time abilities.
 */
void ADedicatedGameMode::ProcessNightResults()
{
    if (!HasAuthority() || !MafiaGameState) return;

    FString KilledPlayerId = TEXT("");
    FString PoliceTargetId = TEXT("");
    FString DetectiveTargetId = TEXT("");

    // Identify primary actions from living special roles
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        AMafiaPlayerState* PS = Cast<AMafiaPlayerState>(It->Get()->PlayerState);
        if (!PS || !PS->GetIsAlive() || PS->GetTargetPlayerId().IsEmpty()) continue;

        FString TargetId = PS->GetTargetPlayerId();
        switch (PS->GetJobType())
        {
        case EJobType::Mafia:     KilledPlayerId = TargetId; break;
        case EJobType::Police:    PoliceTargetId = TargetId; break;
        case EJobType::Detective: DetectiveTargetId = TargetId; break;
        default: break;
        }
    }

    // Process Mafia Kill
    if (!KilledPlayerId.IsEmpty())
    {
        if (AMafiaPlayerState* KilledPS = FindPlayerStateByHash(KilledPlayerId))
        {
            KilledPS->SetAlive(false);
            MafiaGameState->MulticastNotifyPlayerDeath(KilledPlayerId, KilledPS->GetNickName());
            if (KilledPS->GetJobType() != EJobType::Mafia) MafiaGameState->SetCitizenTeamCount(MafiaGameState->GetCitizenTeamCount() - 1);
            MafiaGameState->SetAlivePlayerCount(MafiaGameState->GetAlivePlayerCount() - 1);
            
            for (FConstPlayerControllerIterator VisIt = GetWorld()->GetPlayerControllerIterator(); VisIt; ++VisIt)
            {
                if (AMafiaPlayerState* PS = Cast<AMafiaPlayerState>(VisIt->Get()->PlayerState)) PS->UpdateAllCharacterVisibility();
            }
        }
    }

    // Process Police Investigation
    if (!PoliceTargetId.IsEmpty())
    {
        if (AMafiaPlayerState* TargetPS = FindPlayerStateByHash(PoliceTargetId))
        {
            bool bIsMafia = (TargetPS->GetJobType() == EJobType::Mafia);
            for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
            {
                AMafiaPlayerState* PS = Cast<AMafiaPlayerState>(It->Get()->PlayerState);
                if (PS && PS->GetJobType() == EJobType::Police)
                {
                    PS->ClientNotifyInvestigationResult(TargetPS->GetNickName(), bIsMafia);
                    break;
                }
            }
        }
    }

    // Process Detective Surveillance
    if (!DetectiveTargetId.IsEmpty())
    {
        FString VisitInfo = NightActions.Contains(DetectiveTargetId) ? 
            FString::Printf(TEXT("%s visited %s."), *FindPlayerStateByHash(DetectiveTargetId)->GetNickName(), *FindPlayerStateByHash(NightActions[DetectiveTargetId])->GetNickName()) :
            FString::Printf(TEXT("%s did not visit anyone."), *DetectiveTargetId);

        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            AMafiaPlayerState* PS = Cast<AMafiaPlayerState>(It->Get()->PlayerState);
            if (PS && PS->GetIsAlive() && PS->GetJobType() == EJobType::Detective)
            {
                UpdateMessage(Cast<ADedicatedCharacter>(Cast<APlayerController>(PS->GetOwner())->GetPawn()), -1, 20.0f, FColor::Green, VisitInfo);
                break;
            }
        }
    }

    // Clean up night state
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (AMafiaPlayerState* PS = Cast<AMafiaPlayerState>(It->Get()->PlayerState)) PS->ResetTarget();
    }
    NightActions.Empty();
}

/**
 * @brief [Server RPC] Processes a vote cast by a player during the voting phase.
 */
void ADedicatedGameMode::ServerCastVote_Implementation(const FString& VoterId, const FString& TargetId)
{
    if (!HasAuthority() || !MafiaGameState || MafiaGameState->GetCurrentPhase() != EGamePhase::Voting) return;

    AMafiaPlayerState* VoterPS = FindPlayerStateByHash(VoterId);
    AMafiaPlayerState* TargetPS = FindPlayerStateByHash(TargetId);

    if (!VoterPS || !TargetPS || !VoterPS->GetIsAlive() || !TargetPS->GetIsAlive()) return;

    // Handle vote revocation if player changes their mind
    if (VoteMap.Contains(VoterId))
    {
        if (AMafiaPlayerState* PrevTargetPS = FindPlayerStateByHash(VoteMap[VoterId]))
        {
            int32 NewCount = FMath::Max(0, PrevTargetPS->GetVoteCount() - 1);
            PrevTargetPS->SetVoteCount(NewCount);
            MulticastNotifyVoteUpdate(VoteMap[VoterId], NewCount);
        }
        VoteMap.Remove(VoterId);
    }

    VoteMap.Add(VoterId, TargetId);
    TargetPS->AddVote();
    MulticastNotifyVoteUpdate(TargetId, TargetPS->GetVoteCount());
}

/**
 * @brief Tallys votes and determines if a majority was reached for execution.
 */
void ADedicatedGameMode::ProcessVotingResults()
{
    if (!HasAuthority() || !MafiaGameState) return;

    int32 MajorityVotes = (MafiaGameState->GetAlivePlayerCount() / 2) + 1;
    int32 MaxVotes = 0;
    FString MostVotedPlayerId = TEXT("");
    int32 TieCount = 0;

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        AMafiaPlayerState* PS = Cast<AMafiaPlayerState>(It->Get()->PlayerState);
        if (!PS || !PS->GetIsAlive()) continue;

        if (PS->GetVoteCount() > MaxVotes)
        {
            MaxVotes = PS->GetVoteCount();
            MostVotedPlayerId = PS->GetPlayerHash();
            TieCount = 1;
        }
        else if (PS->GetVoteCount() == MaxVotes && MaxVotes > 0) TieCount++;
    }

    if (MaxVotes >= MajorityVotes && TieCount == 1)
    {
        AMafiaPlayerState* TargetPS = FindPlayerStateByHash(MostVotedPlayerId);
        ALLUpdateMessage(-1, 20.0f, FColor::Green, FString::Printf(TEXT("%s has been selected for execution by majority vote."), *TargetPS->GetNickName()));
        StartLastWordsPhase(MostVotedPlayerId);
    }
    else
    {
        FString ResultText = (TieCount > 1) ? TEXT("No one was executed due to a tie.") : TEXT("No one was executed due to a lack of majority.");
        ALLUpdateMessage(-1, 20.0f, FColor::Green, ResultText);
        
        if (CheckWinCondition()) return;
        MafiaGameState->SetDayCount(MafiaGameState->GetDayCount() + 1);
        StartNightPhase();
    }
}

void ADedicatedGameMode::ServerSubmitLastWords_Implementation(const FString& PlayerId, const FString& Words)
{
    if (!HasAuthority() || !MafiaGameState || MafiaGameState->GetCurrentPhase() != EGamePhase::LastWords) return;
    if (PlayerId == MafiaGameState->GetExecutedPlayerId()) LastWords = Words;
}

/**
 * @brief Evaluates current player counts to determine the winner.
 */
bool ADedicatedGameMode::CheckWinCondition()
{
    if (!HasAuthority() || !MafiaGameState) return false;

    int32 AliveCount = MafiaGameState->GetAlivePlayerCount();
    int32 MafiaCount = MafiaGameState->GetMafiaCount();

    if (MafiaCount == 0) // Citizens win
    {
        MafiaGameState->SetCurrentPhase(EGamePhase::GameOver);
        MafiaGameState->MulticastGameOver(false);
        ALLUpdateMessage(-1, 20.0f, FColor::Green, TEXT("Citizens Win!"));
        return true;
    }

    if (AliveCount == 2 && MafiaCount == 1) // Mafia wins (Majority reached)
    {
        MafiaGameState->SetCurrentPhase(EGamePhase::GameOver);
        MafiaGameState->MulticastGameOver(true);
        ALLUpdateMessage(-1, 20.0f, FColor::Green, TEXT("Mafia Wins!"));
        return true;
    }

    return false;
}

AMafiaPlayerState* ADedicatedGameMode::FindPlayerStateByHash(const FString& PlayerHash)
{
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        AMafiaPlayerState* PS = Cast<AMafiaPlayerState>(It->Get()->PlayerState);
        if (PS && PS->GetPlayerHash() == PlayerHash) return PS;
    }
    return nullptr;
}

/**
 * @brief Broadcasts a debug message to all pawns in the game.
 */
void ADedicatedGameMode::ALLUpdateMessage(int key, float delay, FColor col, const FString& Text)
{
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (ADedicatedCharacter* Char = Cast<ADedicatedCharacter>(It->Get()->GetPawn())) Char->ClientShowMessage(key, delay, col, Text);
    }
}

void ADedicatedGameMode::UpdateMessage(ADedicatedCharacter* Char, int key, float delay, FColor col, const FString& Text)
{
    if (Char) Char->ClientShowMessage(key, delay, col, Text);
}

/**
 * @brief [Server RPC] Handles and filters chat messages based on phase and death status.
 */
void ADedicatedGameMode::ServerSendChatMessage_Implementation(const FString& SenderHash, const FString& Message)
{
    if (!HasAuthority() || !MafiaGameState) return;

    AMafiaPlayerState* SenderPS = FindPlayerStateByHash(SenderHash);
    if (!SenderPS) return;

    // Filter: Accused can chat during Last Words, dead players can only chat with the dead.
    if (MafiaGameState->GetCurrentPhase() == EGamePhase::LastWords && SenderHash != MafiaGameState->GetExecutedPlayerId()) return;

    FLinearColor MessageColor = FLinearColor::White;
    if (MafiaGameState->GetCurrentPhase() == EGamePhase::LastWords) MessageColor = FLinearColor::Red;
    else if (!SenderPS->GetIsAlive()) MessageColor = FLinearColor::Gray;

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        AMafiaPlayerState* ReceiverPS = Cast<AMafiaPlayerState>(It->Get()->PlayerState);
        if (!ReceiverPS) continue;

        // Implementation of 'Ghost Chat' (Dead players only receive messages from other dead players)
        if (SenderPS->GetIsAlive() || (!SenderPS->GetIsAlive() && !ReceiverPS->GetIsAlive()))
        {
            ReceiverPS->ClientReceiveChatMessage(SenderHash, SenderPS->GetNickName(), Message, !SenderPS->GetIsAlive(), MessageColor);
        }
    }
}

/**
 * @brief [Multicast] Toggles movement component for all player characters.
 */
void ADedicatedGameMode::MulticastSetMovementEnabled_Implementation(bool bEnabled)
{
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (ADedicatedCharacter* Char = Cast<ADedicatedCharacter>(It->Get()->GetPawn()))
        {
            if (Char->GetCharacterMovement()) Char->GetCharacterMovement()->SetMovementMode(bEnabled ? MOVE_Walking : MOVE_None);
        }
    }
}

void ADedicatedGameMode::MulticastNotifyVoteUpdate_Implementation(const FString& PlayerId, int32 NewVoteCount)
{
    // Implementation for real-time UI vote counter update on clients
}
