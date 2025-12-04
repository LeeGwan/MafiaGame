// Copyright Epic Games, Inc. All Rights Reserved.
// 마피아 게임 데디케이티드 서버 게임모드 구현

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

// 태양 회전 동기화 (모든 클라이언트)
void ADedicatedGameMode::MulticastUpdateSunRotation_Implementation(FRotator NewRotation)
{
    if (SunLight && SunLight->GetLightComponent())
    {
        SunLight->SetActorRotation(NewRotation);
    }
}

// 낮 시간 설정 (태양 각도 -60도)
void ADedicatedGameMode::SetDayTime()
{
    if (!SunLight) return;

    CurrentSunRotation = FRotator(-60.0f, 0.0f, 0.0f);
    MulticastUpdateSunRotation(CurrentSunRotation);
    //UE_LOG(LogTemp, Warning, TEXT("낮 시간으로 변경"));

}

// 밤 시간 설정 (태양 각도 -130도)
void ADedicatedGameMode::SetNightTime()
{
    if (!SunLight) return;

    CurrentSunRotation = FRotator(-130.0f, 0.0f, 0.0f);
    MulticastUpdateSunRotation(CurrentSunRotation);
    //UE_LOG(LogTemp, Warning, TEXT("밤 시간으로 변경"));
}

// 게임모드 초기화 (게임 상태, 플레이어 상태, 서버 커넥터 설정)
ADedicatedGameMode::ADedicatedGameMode()
{
    ConnectedPlayers = 0;
    LastWords = TEXT("");
    CurrentSunRotation = FRotator(-60.0f, 0.0f, 0.0f);
    PrimaryActorTick.bCanEverTick = true;

    GameStateClass = AMafiaGameState::StaticClass();
    PlayerStateClass = AMafiaPlayerState::StaticClass();
    P_ServerConnector = MakeUnique<ServerConnector>(TEXT("172.30.1.38"), 9050);

}

// 게임 시작 (태양광 찾기, 스폰 위치 초기화, 게임 로비 서버 연결)
void ADedicatedGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        MafiaGameState = Cast<AMafiaGameState>(GameState);

        FindPlayerStarts();

        TArray<AActor*> AllActors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), AllActors);
        for (AActor* Actor : AllActors)
        {
            UDirectionalLightComponent* LightComp = Actor->FindComponentByClass<UDirectionalLightComponent>();

            if (LightComp)
            {
                FString ActorName = Actor->GetName();

                if (ActorName.Contains(TEXT("BP_Directional_Light")))
                {
                    SunLight = Cast<ADirectionalLight>(Actor);

                    if (SunLight)
                    {

                        SetDayTime();
                    }
                    break;
                }
            }
        }

        P_ServerConnector->Start();

    }
}

// 매 프레임 호출
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

// 플레이어 로그인 전처리 (게임 로비 서버에서 인증 정보 검증)
void ADedicatedGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId,
    FString& ErrorMessage)
{
    Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
    FString Hash = UGameplayStatics::ParseOption(Options, TEXT("Hash"));
    FString NickName = UGameplayStatics::ParseOption(Options, TEXT("NickName"));
    FString Unique = UniqueId.ToString();

    UE_LOG(LogTemp, Warning, TEXT("PreLogin  -Args:%s,  - Hash: %s, Name: %s, UniqueId: %s"), *Options, *Hash, *NickName, *Unique);
    if (Hash.IsEmpty())
    {


        return; // 접속 거부
    }

    TempPlayerDataMap.Add(UniqueId.ToString(), TPair<FString, FString>(Hash, NickName));


}

// 플레이어 로그인 후처리 (캐릭터 생성, 플레이어 정보 초기화)
void ADedicatedGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    if (HasAuthority())
    {
        FString UniqueIdStr = NewPlayer->GetPlayerState<APlayerState>()->GetUniqueId().ToString();
        InitPlayerInformaion(NewPlayer, UniqueIdStr);

        UE_LOG(LogTemp, Warning, TEXT("----플레이어 연결: %d/%d-----"), ConnectedPlayers, REQUIRED_PLAYERS);


        if (SunLight)
        {
            MulticastUpdateSunRotation(CurrentSunRotation);
        }

        if (ConnectedPlayers == REQUIRED_PLAYERS && MafiaGameState->GetCurrentPhase() == EGamePhase::Waiting)
        {
            AssignJobs();
            StartGame();
        }
    }
}

// 플레이어 리스폰
void ADedicatedGameMode::RestartPlayer(AController* NewPlayer)
{
    Super::RestartPlayer(NewPlayer);

    if (HasAuthority())
    {

        ADedicatedCharacter* MafiaChar = Cast<ADedicatedCharacter>(NewPlayer->GetPawn());
        AMafiaPlayerState* MafiaPS = Cast<AMafiaPlayerState>(NewPlayer->PlayerState);

        if (MafiaChar && MafiaPS)
        {
            FString PlayerHash = MafiaPS->GetPlayerHash();
            MafiaChar->SetPlayerId(PlayerHash);


        }
    }
}

// 맵에서 플레이어 스폰 포인트 찾기
void ADedicatedGameMode::FindPlayerStarts()
{

    TArray<AActor*> FoundStarts;
    FVector Temp;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), FoundStarts);

    if (FoundStarts.Num() < 6)
    {

        return;
    }


    for (AActor* StartActor : FoundStarts)
    {

        if (StartActor->GetName().Contains(TEXT("PlayerSpwan1_C_17")))
        {
            ExecutionSiteLocation = StartActor->GetActorLocation();


        }
        else
        {
            Temp = StartActor->GetActorLocation();
            SpawnPosition.Enqueue(Temp);

        }
    }

}
// 플레이어 정보 초기화 (세션 토큰, 닉네임, 스폰 위치)
void ADedicatedGameMode::InitPlayerInformaion(APlayerController* NewPlayer, const FString& UniqueIdStr)
{
    if (!HasAuthority() || !MafiaGameState || UniqueIdStr.IsEmpty()) return;


    FVector SpwanPOS;

    AMafiaPlayerState* PS = Cast<AMafiaPlayerState>(NewPlayer->PlayerState);
    ADedicatedCharacter* Char = Cast<ADedicatedCharacter>(NewPlayer->GetPawn());

    if (PS && Char && ConnectedPlayers < REQUIRED_PLAYERS)
    {
        if (SpawnPosition.IsEmpty())
        {
            return;
        }
        if (TempPlayerDataMap.Contains(UniqueIdStr))
        {
            TPair<FString, FString> PlayerData = TempPlayerDataMap[UniqueIdStr];
            FString Hash = PlayerData.Key;
            FString NickName = PlayerData.Value;
            SpawnPosition.Dequeue(SpwanPOS);
            MulticastSetMovementEnabled(true);
            PS->SetPlayerHash(Hash);
            PS->SetNickName(NickName);
            Char->SetPlayerId(Hash);
            Char->SetPlayerName(NickName);
            PS->SetSpawnPosition(SpwanPOS);
            MafiaGameState->SetCurrentPhase(EGamePhase::Waiting);
            Char->ServerMoveToLocation_Implementation(SpwanPOS);
            MafiaGameState->SetPlayerHash(Hash);
            TempPlayerDataMap.Remove(UniqueIdStr);
            ConnectedPlayers++;
        }
    }


}



// 직업 할당 (마피아 2, 경찰 1, 탐정 1, 시민 2)
void ADedicatedGameMode::AssignJobs()
{
    if (!HasAuthority() || !MafiaGameState) return;

    TArray<FString> PlayerHashes = MafiaGameState->GetPlayerHashes();
    if (PlayerHashes.Num() != 6) return;

    TArray<int32> Indices = { 0, 1, 2, 3, 4, 5 };

    for (int32 i = Indices.Num() - 1; i > 0; --i)
    {
        int32 j = FMath::RandRange(0, i);
        Indices.Swap(i, j);
    }

    TArray<EJobType> Jobs = {
        EJobType::Mafia,
        EJobType::Police,
        EJobType::Detective,
        EJobType::Citizen,
        EJobType::Citizen,
        EJobType::Citizen
    };

    int32 MafiaCount = 0;
    int32 CitizenTeamCount = 0;

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        AMafiaPlayerState* PS = Cast<AMafiaPlayerState>(PC->PlayerState);
        ADedicatedCharacter* Char = Cast<ADedicatedCharacter>(PC->GetPawn());
        if (PS)
        {
            FString PlayerHash = PS->GetPlayerHash();

            int32 HashIndex = PlayerHashes.IndexOfByKey(PlayerHash);


            if (HashIndex != INDEX_NONE)
            {
                EJobType AssignedJob = Jobs[Indices[HashIndex]];
                PS->SetJobType(AssignedJob);
                Char->UpdateNameplateWidgetForJobs(PS->GetJobName());
                PS->ClientNotifyJobAssigned(AssignedJob);

                if (AssignedJob == EJobType::Mafia)
                {
                    MafiaCount++;
                }
                else
                {
                    CitizenTeamCount++;
                }

            }
        }
    }

    MafiaGameState->SetMafiaCount(MafiaCount);
    MafiaGameState->SetCitizenTeamCount(CitizenTeamCount);
    MafiaGameState->SetAlivePlayerCount(6);


}

// 모든 플레이어를 시작 위치로 이동 (멀티캐스트)
void ADedicatedGameMode::MulticastMovePlayersToStart_Implementation()
{
    if (!MafiaGameState) return;

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        AMafiaPlayerState* PS = Cast<AMafiaPlayerState>(PC->PlayerState);
        ADedicatedCharacter* Char = Cast<ADedicatedCharacter>(PC->GetPawn());

        if (PS && Char && PS->GetIsAlive())
        {
            FString PlayerId = PS->GetPlayerHash();
            FVector TargetLocation = PS->GetSpawnPosition();

            if (!TargetLocation.IsZero())
            {
                Char->ForceMoveToLocation(TargetLocation);

            }
        }
    }
}
// 게임 시작 (6명 모두 접속 시)
void ADedicatedGameMode::StartGame()
{
    if (!HasAuthority() || !MafiaGameState) return;
    MafiaGameState->SetDayCount(1);
    StartMorningPhase();
}

// 밤 페이즈 시작 (마피아/경찰/탐정 행동)
void ADedicatedGameMode::StartNightPhase()
{
    if (!HasAuthority() || !MafiaGameState) return;

    SetNightTime();

    MafiaGameState->SetCurrentPhase(EGamePhase::Night);
    MafiaGameState->SetPhaseTimer(NIGHT_DURATION);

    MulticastSetMovementEnabled(false);
    MafiaGameState->MulticastPhaseChanged(EGamePhase::Night, NIGHT_DURATION);

    GetWorld()->GetTimerManager().SetTimer(PhaseTimerHandle, this, &ADedicatedGameMode::OnPhaseTimeEnd, NIGHT_DURATION, false);
    FString text = FString::Printf(TEXT("===== 밤 시작 (2분) - 이동 불가 ====="));
    ALLUpdateMessage(-1, 20.0f, FColor::Green, text);
    UE_LOG(LogTemp, Warning, TEXT("===== 밤 시작 (2분) - 이동 불가 ====="));
}
// 플레이어 대기 페이즈
auto ADedicatedGameMode::WaitPlayerPhase()
{
    if (!HasAuthority() || !MafiaGameState) return;



    MafiaGameState->SetCurrentPhase(EGamePhase::Waiting);


    MulticastSetMovementEnabled(true);


    //UE_LOG(LogTemp, Warning, TEXT("플레이어 기달리는중"));
}
// 아침 페이즈 시작 (밤 결과 공개)
void ADedicatedGameMode::StartMorningPhase()
{
    if (!HasAuthority() || !MafiaGameState) return;

    SetDayTime();

    if (MafiaGameState->GetDayCount() != 1)
    {
        ProcessNightResults();
    }

    if (CheckWinCondition())
    {

        return;
    }
    MafiaGameState->SetCurrentPhase(EGamePhase::Morning);
    MafiaGameState->SetPhaseTimer(MORNING_DURATION);

    MulticastSetMovementEnabled(true);
    MafiaGameState->MulticastPhaseChanged(EGamePhase::Morning, MORNING_DURATION);

    GetWorld()->GetTimerManager().SetTimer(PhaseTimerHandle, this, &ADedicatedGameMode::OnPhaseTimeEnd, MORNING_DURATION, false);
    FString text = FString::Printf(TEXT("===== 아침/토론 시작 (5분) - 이동 가능 ====="));
    ALLUpdateMessage(-1, 20.0f, FColor::Green, text);

    UE_LOG(LogTemp, Warning, TEXT("===== 아침/토론 시작 (5분) - 이동 가능 ====="));
}

// 투표 페이즈 시작
void ADedicatedGameMode::StartVotingPhase()
{
    if (!HasAuthority() || !MafiaGameState) return;

    SetDayTime();
    MafiaGameState->SetCurrentPhase(EGamePhase::Voting);
    MafiaGameState->SetPhaseTimer(VOTING_DURATION);
    VoteMap.Empty();

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        AMafiaPlayerState* PS = Cast<AMafiaPlayerState>(It->Get()->PlayerState);
        if (PS)
        {
            PS->ResetVotes();
        }
    }

    MulticastSetMovementEnabled(false);
    MulticastMovePlayersToStart();

    MafiaGameState->MulticastPhaseChanged(EGamePhase::Voting, VOTING_DURATION);

    GetWorld()->GetTimerManager().SetTimer(PhaseTimerHandle, this, &ADedicatedGameMode::OnPhaseTimeEnd, VOTING_DURATION, false);
    FString text = FString::Printf(TEXT("===== 투표 시작 (2분) - PlayerStart로 이동, 이동 불가 ====="));
    ALLUpdateMessage(-1, 20.0f, FColor::Green, text);

    UE_LOG(LogTemp, Warning, TEXT("===== 투표 시작 (2분) - PlayerStart로 이동, 이동 불가 ====="));
}

// 유언 페이즈 시작
void ADedicatedGameMode::StartLastWordsPhase(const FString& PlayerId)
{
    if (!HasAuthority() || !MafiaGameState) return;

    SetDayTime();
    MafiaGameState->SetCurrentPhase(EGamePhase::LastWords);
    MafiaGameState->SetPhaseTimer(LASTWORDS_DURATION);
    MafiaGameState->SetExecutedPlayerId(PlayerId);
    LastWords = TEXT("");

    MafiaGameState->MulticastPhaseChanged(EGamePhase::LastWords, LASTWORDS_DURATION);

    // 처형 대상자를 ExecutionSite로 이동
    AMafiaPlayerState* ExecutedPS = FindPlayerStateByHash(PlayerId);
    if (ExecutedPS)
    {
        APlayerController* PC = Cast<APlayerController>(ExecutedPS->GetOwner());
        if (PC)
        {
            ADedicatedCharacter* Char = Cast<ADedicatedCharacter>(PC->GetPawn());
            if (Char && !ExecutionSiteLocation.IsZero())
            {
                Char->ForceMoveToLocation(ExecutionSiteLocation);

                FString text = FString::Printf(TEXT("%s 님이 처형대로 이동"), *Char->GetPlayerName());
                ALLUpdateMessage(-1, 20.0f, FColor::Green, text);
                //UE_LOG(LogTemp, Warning, TEXT("플레이어 %s를 ExecutionSite로 이동"), *PlayerId);
            }

        }
    }

    GetWorld()->GetTimerManager().SetTimer(PhaseTimerHandle, this, &ADedicatedGameMode::OnPhaseTimeEnd, LASTWORDS_DURATION, false);

    UE_LOG(LogTemp, Warning, TEXT("===== 플레이어 %s 유언 시간 (10초) ====="), *PlayerId);
}

// 처형 실행
void ADedicatedGameMode::ProcessExecution()
{
    if (!HasAuthority() || !MafiaGameState) return;

    FString ExecutedPlayerId = MafiaGameState->GetExecutedPlayerId();
    if (ExecutedPlayerId.IsEmpty()) return;

    AMafiaPlayerState* ExecutedPS = FindPlayerStateByHash(ExecutedPlayerId);
    if (!ExecutedPS) return;

    FString ExcutedPlayerNickName = ExecutedPS->GetNickName();
    ExecutedPS->SetAlive(false);
    FString text = FString::Printf(TEXT("플레이어 %s 처형됨 직업: %s"), *ExcutedPlayerNickName, *ExecutedPS->GetJobName());
    ALLUpdateMessage(-1, 20.0f, FColor::Green, text);
    UE_LOG(LogTemp, Warning, TEXT("유언: %s"), *LastWords);

    MafiaGameState->MulticastNotifyPlayerDeath(ExecutedPlayerId, ExecutedPS->GetNickName());
    MafiaGameState->MulticastNotifyLastWords(ExecutedPS->GetNickName(), LastWords);

    // 모든 플레이어의 visibility 업데이트
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {

        AMafiaPlayerState* PS = Cast<AMafiaPlayerState>(It->Get()->PlayerState);
        if (PS)
        {
            PS->UpdateAllCharacterVisibility();
        }
    }

    if (ExecutedPS->GetJobType() == EJobType::Mafia)
    {
        MafiaGameState->SetMafiaCount(MafiaGameState->GetMafiaCount() - 1);
    }
    else
    {
        MafiaGameState->SetCitizenTeamCount(MafiaGameState->GetCitizenTeamCount() - 1);
    }

    MafiaGameState->SetAlivePlayerCount(MafiaGameState->GetAlivePlayerCount() - 1);
    LastWords = TEXT("");

    if (CheckWinCondition()) return;

    MafiaGameState->SetDayCount(MafiaGameState->GetDayCount() + 1);
    StartNightPhase();
}

// 페이즈 타이머 종료 시 호출
void ADedicatedGameMode::OnPhaseTimeEnd()
{
    if (!HasAuthority() || !MafiaGameState) return;

    switch (MafiaGameState->GetCurrentPhase())
    {
    case EGamePhase::Night:
        StartMorningPhase();
        break;

    case EGamePhase::Morning:
        StartVotingPhase();
        break;

    case EGamePhase::Voting:
        ProcessVotingResults();
        break;

    case EGamePhase::LastWords:
        ProcessExecution();
        break;

    default:
        break;
    }
}

// 밤 행동 처리 (서버 RPC)
void ADedicatedGameMode::ServerProcessNightAction_Implementation(const FString& PlayerId, const FString& TargetId)
{
    if (!HasAuthority() || !MafiaGameState)
    {
        //UE_LOG(LogTemp, Warning, TEXT("ServerProcessNightAction_Implementation !HasAuthority() || !MafiaGameState"));
        return;
    }
    if (MafiaGameState->GetCurrentPhase() != EGamePhase::Night)
    {
        //UE_LOG(LogTemp, Warning, TEXT("ServerProcessNightAction_Implementation it isn't night"));
        return;
    }
    AMafiaPlayerState* PS = FindPlayerStateByHash(PlayerId);
    if (!PS || !PS->GetIsAlive() || !PS->CanNightAction())
    {
        return;
    }
    AMafiaPlayerState* TPS = FindPlayerStateByHash(TargetId);
    FString text;
    text.Empty();
    if (TPS && TPS->GetIsAlive())
    {
        text = FString::Printf(TEXT("플레이어 %s를 선택"), *TPS->GetNickName());
    }
    if (NightActions.Contains(PlayerId))
    {
        NightActions.Remove(PlayerId);
    }

    PS->SetTarget(TargetId);
    NightActions.Add(PlayerId, TargetId);
    APlayerController* PC = Cast<APlayerController>(PS->GetOwner());
    if (PC)
    {
        ADedicatedCharacter* Char = Cast<ADedicatedCharacter>(PC->GetPawn());
        UpdateMessage(Char, -1, 20.0f, FColor::Green, text);
    }

    UE_LOG(LogTemp, Warning, TEXT("플레이어 %s가 플레이어 %s를 선택"), *PlayerId, *TargetId);
}
// 밤 결과 처리 (마피아 공격, 경찰 조사, 탐정 조사)
void ADedicatedGameMode::ProcessNightResults()
{
    if (!HasAuthority() || !MafiaGameState) return;

    FString KilledPlayerId = TEXT("");
    FString PoliceTargetId = TEXT("");
    FString DetectiveTargetId = TEXT("");

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        AMafiaPlayerState* PS = Cast<AMafiaPlayerState>(It->Get()->PlayerState);
        if (!PS || !PS->GetIsAlive()) continue;

        FString TargetId = PS->GetTargetPlayerId();
        if (TargetId.IsEmpty()) continue;

        switch (PS->GetJobType())
        {
        case EJobType::Mafia:
            KilledPlayerId = TargetId;
            break;
        case EJobType::Police:
            PoliceTargetId = TargetId;
            break;
        case EJobType::Detective:
            DetectiveTargetId = TargetId;
            break;
        default:
            break;
        }
    }

    //UE_LOG(LogTemp, Warning, TEXT("===== %d일차 아침 ====="), MafiaGameState->GetDayCount());

    if (!KilledPlayerId.IsEmpty())
    {
        AMafiaPlayerState* KilledPS = FindPlayerStateByHash(KilledPlayerId);
        if (KilledPS)
        {
            KilledPS->SetAlive(false);
            MafiaGameState->MulticastNotifyPlayerDeath(KilledPlayerId, KilledPS->GetNickName());

            if (KilledPS->GetJobType() != EJobType::Mafia)
            {
                MafiaGameState->SetCitizenTeamCount(MafiaGameState->GetCitizenTeamCount() - 1);
            }
            MafiaGameState->SetAlivePlayerCount(MafiaGameState->GetAlivePlayerCount() - 1);

            // 모든 플레이어의 visibility 업데이트
            for (FConstPlayerControllerIterator VisIt = GetWorld()->GetPlayerControllerIterator(); VisIt; ++VisIt)
            {
                AMafiaPlayerState* PS = Cast<AMafiaPlayerState>(VisIt->Get()->PlayerState);
                if (PS)
                {
                    PS->UpdateAllCharacterVisibility();
                }
            }
        }
    }

    if (!PoliceTargetId.IsEmpty())
    {
        AMafiaPlayerState* TargetPS = FindPlayerStateByHash(PoliceTargetId);
        if (TargetPS)
        {
            bool bIsMafia = (TargetPS->GetJobType() == EJobType::Mafia);
            FString TargetNickName = TargetPS->GetNickName();
            for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
            {
                AMafiaPlayerState* PS = Cast<AMafiaPlayerState>(It->Get()->PlayerState);
                if (PS && PS->GetJobType() == EJobType::Police)
                {
                    PS->SetInvestigationResult(bIsMafia);
                    PS->ClientNotifyInvestigationResult(TargetNickName, bIsMafia);
                    break;
                }
            }
        }
    }
    AMafiaPlayerState* DetectivePS = nullptr;
    AMafiaPlayerState* VisitedIdPS = nullptr;
    if (!DetectiveTargetId.IsEmpty())
    {
        FString VisitInfo;

        if (NightActions.Contains(DetectiveTargetId))
        {
            FString VisitedId = NightActions[DetectiveTargetId];
            DetectivePS = FindPlayerStateByHash(DetectiveTargetId);
            VisitedIdPS = FindPlayerStateByHash(VisitedId);
            if (DetectivePS && VisitedIdPS)
                VisitInfo = FString::Printf(TEXT("플레이어 %s가 플레이어 %s를 방문했습니다."),
                    *DetectivePS->GetNickName(), *VisitedIdPS->GetNickName());
        }
        else
        {
            VisitInfo = FString::Printf(TEXT("플레이어 %s는 아무도 방문하지 않았습니다."),
                *DetectiveTargetId);
        }

        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            AMafiaPlayerState* PS = Cast<AMafiaPlayerState>(It->Get()->PlayerState);
            if (!PS || !PS->GetIsAlive())continue;

            APlayerController* PC = Cast<APlayerController>(PS->GetOwner());
            if (!PC)continue;

            ADedicatedCharacter* Char = Cast<ADedicatedCharacter>(PC->GetPawn());
            if (PS->GetJobType() == EJobType::Detective)
            {
                PS->SetVisitInfo(VisitInfo);
                UpdateMessage(Char, -1, 20.0f, FColor::Green, VisitInfo);
                break;
            }
        }
    }

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        AMafiaPlayerState* PS = Cast<AMafiaPlayerState>(It->Get()->PlayerState);
        if (PS)
        {
            PS->ResetTarget();
        }
    }
    NightActions.Empty();
}


// 투표 처리 (서버 RPC)
void ADedicatedGameMode::ServerCastVote_Implementation(const FString& VoterId, const FString& TargetId)
{
    if (!HasAuthority() || !MafiaGameState)
    {
        //UE_LOG(LogTemp, Warning, TEXT("!HasAuthority() || !MafiaGameState"));
        return;
    }
    if (MafiaGameState->GetCurrentPhase() != EGamePhase::Voting)
    {
        //UE_LOG(LogTemp, Warning, TEXT("!= EGamePhase::Voting"));
        return;
    }
    AMafiaPlayerState* VoterPS = FindPlayerStateByHash(VoterId);
    AMafiaPlayerState* TargetPS = FindPlayerStateByHash(TargetId);

    if (!VoterPS || !TargetPS)
    {
        if (!VoterPS)
        {
            //UE_LOG(LogTemp, Warning, TEXT("!VoterPS"));
        }
        if (!TargetPS)
        {
            //UE_LOG(LogTemp, Warning, TEXT("!TargetPS"));
        }
        return;
    }
    if (!VoterPS->GetIsAlive() || !TargetPS->GetIsAlive())
    {
        if (!VoterPS->GetIsAlive())
        {
            UE_LOG(LogTemp, Warning, TEXT("!VoterPS->GetIsAlive()"));
        }
        if (!TargetPS->GetIsAlive())
        {
            UE_LOG(LogTemp, Warning, TEXT("!TargetPS->GetIsAlive()"));
        }
        return;
    }
    if (VoteMap.Contains(VoterId))
    {
        FString PreviousTargetId = VoteMap[VoterId];
        AMafiaPlayerState* PreviousTargetPS = FindPlayerStateByHash(PreviousTargetId);

        if (PreviousTargetPS)
        {
            int32 NewVoteCount = FMath::Max(0, PreviousTargetPS->GetVoteCount() - 1);
            PreviousTargetPS->SetVoteCount(NewVoteCount);

            MulticastNotifyVoteUpdate(PreviousTargetId, NewVoteCount);
            UE_LOG(LogTemp, Warning, TEXT("플레이어 %s의 이전 투표 취소 (타겟: %s)"), *VoterId, *PreviousTargetId);
        }

        VoteMap.Remove(VoterId);
    }

    VoteMap.Add(VoterId, TargetId);
    TargetPS->AddVote();

    MulticastNotifyVoteUpdate(TargetId, TargetPS->GetVoteCount());
    //UpdateMessage() 
    UE_LOG(LogTemp, Warning, TEXT("플레이어 %s가 플레이어 %s에게 투표 (총 %d표)"),
        *VoterId, *TargetId, TargetPS->GetVoteCount());
}

// 투표 결과 집계 (최다 득표자 처형)
void ADedicatedGameMode::ProcessVotingResults()
{
    if (!HasAuthority() || !MafiaGameState) return;

    int32 AliveCount = MafiaGameState->GetAlivePlayerCount();
    int32 MajorityVotes = (AliveCount / 2) + 1;
    int32 MaxVotes = 0;
    FString MostVotedPlayerId = TEXT("");
    int32 TieCount = 0;

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        AMafiaPlayerState* PS = Cast<AMafiaPlayerState>(It->Get()->PlayerState);
        if (!PS || !PS->GetIsAlive()) continue;

        int32 Votes = PS->GetVoteCount();

        if (Votes > MaxVotes)
        {
            MaxVotes = Votes;
            MostVotedPlayerId = PS->GetPlayerHash();
            TieCount = 1;
        }
        else if (Votes == MaxVotes && Votes > 0)
        {
            TieCount++;
        }
    }

    //UE_LOG(LogTemp, Warning, TEXT("투표 결과: 최다 득표 %d표"), MaxVotes);

    if (MaxVotes >= MajorityVotes && TieCount == 1)
    {

        AMafiaPlayerState* MaxVotesPS = FindPlayerStateByHash(MostVotedPlayerId);
        if (MaxVotesPS)
        {
            FString text = FString::Printf(TEXT("플레이어 %s가 과반수 득표로 처형 대상"), *MaxVotesPS->GetNickName());

            ALLUpdateMessage(-1, 20.0f, FColor::Green, text);
        }
        //UE_LOG(LogTemp, Warning, TEXT("플레이어 %s가 과반수 득표로 처형 대상"), *MostVotedPlayerId);
        StartLastWordsPhase(MostVotedPlayerId);
    }
    else
    {
        FString text;
        if (TieCount > 1)
        {
            text = FString::Printf(TEXT("동점으로 아무도 처형되지 않음"));

        }
        else
        {
            text = FString::Printf(TEXT("과반수 미달로 아무도 처형되지 않음"));

        }
        ALLUpdateMessage(-1, 20.0f, FColor::Green, text);
        if (CheckWinCondition())
        {
            //disconnect to server
            return;
        }

        MafiaGameState->SetDayCount(MafiaGameState->GetDayCount() + 1);
        StartNightPhase();
    }
}

// 유언 제출 (서버 RPC)
void ADedicatedGameMode::ServerSubmitLastWords_Implementation(const FString& PlayerId, const FString& Words)
{
    if (!HasAuthority() || !MafiaGameState) return;
    if (MafiaGameState->GetCurrentPhase() != EGamePhase::LastWords) return;
    if (PlayerId != MafiaGameState->GetExecutedPlayerId()) return;

    LastWords = Words;
    //UE_LOG(LogTemp, Warning, TEXT("플레이어 %s의 유언: %s"), *PlayerId, *Words);
}

// 승리 조건 체크 (마피아 수 vs 시민팀 수)
bool ADedicatedGameMode::CheckWinCondition()
{
    if (!HasAuthority() || !MafiaGameState) return false;

    int32 AliveCount = MafiaGameState->GetAlivePlayerCount();
    int32 MafiaCount = MafiaGameState->GetMafiaCount();

    if (MafiaCount == 0)
    {
        MafiaGameState->SetCurrentPhase(EGamePhase::GameOver);
        MafiaGameState->MulticastGameOver(false);
        FString text = FString::Printf(TEXT("시민팀 승리!"));
        ALLUpdateMessage(-1, 20.0f, FColor::Green, text);

        //UE_LOG(LogTemp, Warning, TEXT("시민팀 승리!"));
        return true;
    }

    if (AliveCount == 2 && MafiaCount == 1)
    {
        MafiaGameState->SetCurrentPhase(EGamePhase::GameOver);
        MafiaGameState->MulticastGameOver(true);
        FString text = FString::Printf(TEXT("마피아 승리!"));
        ALLUpdateMessage(-1, 20.0f, FColor::Green, text);

        //UE_LOG(LogTemp, Warning, TEXT("마피아 승리!"));
        return true;
    }

    return false;
}

AMafiaPlayerState* ADedicatedGameMode::FindPlayerStateByHash(const FString& PlayerHash)
{
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        AMafiaPlayerState* PS = Cast<AMafiaPlayerState>(It->Get()->PlayerState);
        if (PS && PS->GetPlayerHash() == PlayerHash)
        {
            return PS;
        }
    }
    return nullptr;
}

// 모든 플레이어에게 메시지 전송
void ADedicatedGameMode::ALLUpdateMessage(int key, float delay, FColor col, const FString& Text)
{
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        if (PC)
        {
            ADedicatedCharacter* Char = Cast<ADedicatedCharacter>(PC->GetPawn());
            if (Char)
            {
                Char->ClientShowMessage(key, delay, col, Text);
            }
        }
    }

}

// 특정 플레이어에게 메시지 전송
void ADedicatedGameMode::UpdateMessage(ADedicatedCharacter* Char, int key, float delay, FColor col, const FString& Text)
{
    if (Char)
    {
        Char->ClientShowMessage(key, delay, col, Text);
    }
}


// 채팅 메시지 처리 (서버 RPC)
void ADedicatedGameMode::ServerSendChatMessage_Implementation(const FString& SenderHash, const FString& Message)
{
    if (!HasAuthority() || !MafiaGameState) return;

    AMafiaPlayerState* SenderPS = FindPlayerStateByHash(SenderHash);
    if (!SenderPS) return;

    bool bSenderIsAlive = SenderPS->GetIsAlive();
    FString SenderName = SenderPS->GetPlayerName();

    EGamePhase CurrentPhase = MafiaGameState->GetCurrentPhase();

    if (CurrentPhase == EGamePhase::LastWords)
    {
        FString ExecutedPlayerId = MafiaGameState->GetExecutedPlayerId();
        if (SenderHash != ExecutedPlayerId)
        {
            return;
        }
    }

    FLinearColor MessageColor;

    if (CurrentPhase == EGamePhase::LastWords && SenderHash == MafiaGameState->GetExecutedPlayerId())
    {
        MessageColor = FLinearColor::Red;
    }
    else if (!bSenderIsAlive)
    {
        MessageColor = FLinearColor::Gray;
    }
    else
    {
        MessageColor = FLinearColor::White;
    }

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        AMafiaPlayerState* ReceiverPS = Cast<AMafiaPlayerState>(PC->PlayerState);

        if (!ReceiverPS) continue;

        bool bReceiverIsAlive = ReceiverPS->GetIsAlive();

        bool bShouldReceive = false;

        if (bSenderIsAlive)
        {
            bShouldReceive = true;
        }
        else
        {
            if (!bReceiverIsAlive)
            {
                bShouldReceive = true;
            }
        }

        if (bShouldReceive)
        {
            ReceiverPS->ClientReceiveChatMessage(SenderHash, SenderName, Message, !bSenderIsAlive, MessageColor);
        }
    }
}

// 플레이어 이동 활성화/비활성화 (멀티캐스트)
void ADedicatedGameMode::MulticastSetMovementEnabled_Implementation(bool bEnabled)
{
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        ADedicatedCharacter* Char = Cast<ADedicatedCharacter>(PC->GetPawn());

        if (Char && Char->GetCharacterMovement())
        {
            Char->GetCharacterMovement()->SetMovementMode(bEnabled ? MOVE_Walking : MOVE_None);
        }
    }

    //UE_LOG(LogTemp, Warning, TEXT("이동 %s"), bEnabled ? TEXT("활성화") : TEXT("비활성화"));
}



// 투표 업데이트 알림 (멀티캐스트)
void ADedicatedGameMode::MulticastNotifyVoteUpdate_Implementation(const FString& PlayerId, int32 NewVoteCount)
{

    //UE_LOG(LogTemp, Warning, TEXT("플레이어 %s 투표 수: %d"), *PlayerId, NewVoteCount);
}