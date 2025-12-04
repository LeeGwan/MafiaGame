// Fill out your copyright notice in the Description page of Project Settings.

#include "MafiaPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "DedicatedCharacter.h"

AMafiaPlayerState::AMafiaPlayerState()
{
    PlayerHash = TEXT("");
    JobType = EJobType::None;
    bIsAlive = true;
    VoteCount = 0;
    TargetPlayerId = TEXT("");
    bInvestigationResult = false;
    VisitInfo = TEXT("");
    SpawnPosition = { 0.0f,0.0f,0.0f };
}

// 네트워크 복제 속성 등록
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

// 직업명 반환
FString AMafiaPlayerState::GetJobName() const
{
    switch (JobType)
    {
    case EJobType::Mafia: return TEXT("Mafia");
    case EJobType::Police: return TEXT("Police");
    case EJobType::Detective: return TEXT("Detective");
    case EJobType::Citizen: return TEXT("Citizen");
    default: return TEXT("None");
    }
}

// 밤 행동 가능 여부 확인 (마피아/경찰/탐정만 가능)
bool AMafiaPlayerState::CanNightAction()
{
    switch (JobType)
    {
    case EJobType::Mafia: return true;
    case EJobType::Police: return true;
    case EJobType::Detective: return true;
    default: return false;
    }
}

// 스폰 위치 설정 (서버 전용)
void AMafiaPlayerState::SetSpawnPosition(const FVector& POS)
{
    if (HasAuthority())
    {
        SpawnPosition = POS;
    }
}

// 플레이어 세션 토큰 설정 (서버 전용)
void AMafiaPlayerState::SetPlayerHash(const FString& Hash)
{
    if (HasAuthority())
    {
        PlayerHash = Hash;
    }
}

// 닉네임 설정 (서버 전용)
void AMafiaPlayerState::SetNickName(const FString& NickName)
{
    if (HasAuthority())
    {
        PlayerNickName = NickName;
    }
}

// 직업 설정 (서버 전용)
void AMafiaPlayerState::SetJobType(EJobType NewJobType)
{
    if (HasAuthority())
    {
        JobType = NewJobType;
    }
}

// 생존 상태 설정 (서버 전용)
void AMafiaPlayerState::SetAlive(bool bAlive)
{
    if (HasAuthority())
    {
        bIsAlive = bAlive;

        if (!bIsAlive)
        {
            UpdateAllCharacterVisibility();
        }
    }
}

// 투표 수 설정 (서버 전용)
void AMafiaPlayerState::SetVoteCount(int32 Count)
{
    if (HasAuthority())
    {
        VoteCount = Count;
    }
}

// 투표 추가 (서버 전용)
void AMafiaPlayerState::AddVote()
{
    if (HasAuthority())
    {
        VoteCount++;
    }
}

// 투표 초기화 (서버 전용)
void AMafiaPlayerState::ResetVotes()
{
    if (HasAuthority())
    {
        VoteCount = 0;
    }
}

// 대상 설정 (서버 전용)
void AMafiaPlayerState::SetTarget(const FString& TargetId)
{
    if (HasAuthority())
    {
        TargetPlayerId = TargetId;
    }
}

// 대상 초기화 (서버 전용)
void AMafiaPlayerState::ResetTarget()
{
    if (HasAuthority())
    {
        TargetPlayerId = TEXT("");
    }
}

// 경찰 조사 결과 설정 (서버 전용)
void AMafiaPlayerState::SetInvestigationResult(bool bIsMafia)
{
    if (HasAuthority())
    {
        bInvestigationResult = bIsMafia;
    }
}

// 탐정 방문자 정보 설정 (서버 전용)
void AMafiaPlayerState::SetVisitInfo(const FString& Info)
{
    if (HasAuthority())
    {
        VisitInfo = Info;
    }
}

// 생존 상태 변경 시 호출 (네트워크 복제 콜백)
void AMafiaPlayerState::OnRep_IsAlive()
{
    UpdateAllCharacterVisibility();
}

// 모든 캐릭터 가시성 업데이트 (죽은 플레이어는 보이지 않음)
void AMafiaPlayerState::UpdateAllCharacterVisibility()
{
    UWorld* World = GetWorld();
    if (!World) return;

    APlayerController* MyPC = Cast<APlayerController>(GetOwner());
    if (!MyPC) return;

    AMafiaPlayerState* MyPS = Cast<AMafiaPlayerState>(MyPC->PlayerState);
    if (!MyPS) return;

    bool bIAmAlive = MyPS->GetIsAlive();

    // 모든 플레이어 순회
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        if (!PC) continue;

        AMafiaPlayerState* PS = Cast<AMafiaPlayerState>(PC->PlayerState);
        if (!PS) continue;

        APawn* Pawn = PC->GetPawn();
        if (!Pawn) continue;

        // 내가 살아있으면 죽은 사람만 숨김, 내가 죽었으면 모두 보임
        if (bIAmAlive)
        {
            if (!PS->GetIsAlive())
            {
                Pawn->SetActorHiddenInGame(true);
            }
            else
            {
                Pawn->SetActorHiddenInGame(false);
            }
        }
        else
        {
            Pawn->SetActorHiddenInGame(false);
        }
    }
}

// 채팅 메시지 수신 (클라이언트 RPC)
void AMafiaPlayerState::ClientReceiveChatMessage_Implementation(const FString& SenderHash, const FString& SenderName,
    const FString& Message, bool bSenderIsDead, FLinearColor MessageColor)
{
    OnChatMessageReceived.Broadcast(SenderHash, SenderName, Message, bSenderIsDead, MessageColor);
}

// 직업 할당 알림 (클라이언트 RPC)
void AMafiaPlayerState::ClientNotifyJobAssigned_Implementation(EJobType AssignedJob)
{
    GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green,
        FString::Printf(TEXT("[Client] 직업 할당: %s"), *GetJobName()));
}

// 경찰 조사 결과 알림 (클라이언트 RPC)
void AMafiaPlayerState::ClientNotifyInvestigationResult_Implementation(const FString& TargetId, bool bIsMafia)
{
    GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green,
        FString::Printf(TEXT("[경찰] 플레이어 %s는 %s"),
            *TargetId, bIsMafia ? TEXT("마피아입니다!") : TEXT("마피아가 아닙니다.")));
}

// 탐정 조사 결과 알림 (클라이언트 RPC)
void AMafiaPlayerState::ClientNotifyVisitInfo_Implementation(const FString& Info)
{
    GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green,
        FString::Printf(TEXT("[탐정] %s"), *Info));
}