// MafiaGameState.cpp
#include "MafiaGameState.h"
#include "Net/UnrealNetwork.h"

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

// 네트워크 복제 속성 등록
void AMafiaGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AMafiaGameState, CurrentPhase);
    DOREPLIFETIME(AMafiaGameState, PhaseTimer);
    DOREPLIFETIME(AMafiaGameState, DayCount);
    DOREPLIFETIME(AMafiaGameState, MafiaCount);
    DOREPLIFETIME(AMafiaGameState, CitizenTeamCount);
    DOREPLIFETIME(AMafiaGameState, AlivePlayerCount);
    DOREPLIFETIME(AMafiaGameState, ExecutedPlayerId);
    DOREPLIFETIME(AMafiaGameState, PlayerHashes);
    DOREPLIFETIME(AMafiaGameState, PlayerStartLocations);
}

// 현재 페이즈 설정 (서버 전용)
void AMafiaGameState::SetCurrentPhase(EGamePhase NewPhase)
{
    if (HasAuthority())
    {
        CurrentPhase = NewPhase;
    }
}

// 페이즈 타이머 설정 (서버 전용)
void AMafiaGameState::SetPhaseTimer(float NewTimer)
{
    if (HasAuthority())
    {
        PhaseTimer = NewTimer;
    }
}

// 날짜 설정 (서버 전용)
void AMafiaGameState::SetDayCount(int32 NewDay)
{
    if (HasAuthority())
    {
        DayCount = NewDay;
    }
}

// 마피아 생존자 수 설정 (서버 전용)
void AMafiaGameState::SetMafiaCount(int32 Count)
{
    if (HasAuthority())
    {
        MafiaCount = Count;
    }
}

// 시민팀 생존자 수 설정 (서버 전용)
void AMafiaGameState::SetCitizenTeamCount(int32 Count)
{
    if (HasAuthority())
    {
        CitizenTeamCount = Count;
    }
}

// 총 생존자 수 설정 (서버 전용)
void AMafiaGameState::SetAlivePlayerCount(int32 Count)
{
    if (HasAuthority())
    {
        AlivePlayerCount = Count;
    }
}

// 처형된 플레이어 ID 설정 (서버 전용)
void AMafiaGameState::SetExecutedPlayerId(const FString& PlayerId)
{
    if (HasAuthority())
    {
        ExecutedPlayerId = PlayerId;
    }
}

// 플레이어 세션 토큰 목록 설정 (서버 전용)
void AMafiaGameState::SetPlayerHashes(const TArray<FString>& Hashes)
{
    if (HasAuthority())
    {
        PlayerHashes = Hashes;
    }
}

// 플레이어 세션 토큰 추가 (서버 전용)
void AMafiaGameState::SetPlayerHash(const FString& Hashes)
{
    if (HasAuthority())
    {
        PlayerHashes.Add(Hashes);
    }
}

// 특정 인덱스의 플레이어 세션 토큰 설정 (서버 전용)
void AMafiaGameState::SetAtPlayerHash(int index, const FString& Hashes)
{
    if (HasAuthority())
    {
        PlayerHashes[index] = Hashes;
    }
}

// 플레이어 스폰 위치 추가 (서버 전용)
void AMafiaGameState::AddPlayerStartLocation(const FString& PlayerHash, FVector Location)
{
    if (HasAuthority())
    {
        PlayerStartLocations.Add(FPlayerStartData(PlayerHash, Location));
    }
}

// 플레이어 사망 알림 (모든 클라이언트)
void AMafiaGameState::MulticastNotifyPlayerDeath_Implementation(const FString& PlayerId, const FString& PlayerName)
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red,
            FString::Printf(TEXT("플레이어 %s가 사망했습니다"), *PlayerName));
    }

    UE_LOG(LogTemp, Warning, TEXT("플레이어 %s (%s) 사망"), *PlayerId, *PlayerName);
}

// 플레이어 세션 토큰으로 스폰 위치 찾기
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

// 페이즈 변경 알림 (모든 클라이언트)
void AMafiaGameState::MulticastPhaseChanged_Implementation(EGamePhase NewPhase, float Duration)
{
    CurrentPhase = NewPhase;
    PhaseTimer = Duration;

    UE_LOG(LogTemp, Warning, TEXT("[GameState] 페이즈 변경: %d, 시간: %.0f초"), (int32)NewPhase, Duration);
}

// 게임 종료 알림 (모든 클라이언트)
void AMafiaGameState::MulticastGameOver_Implementation(bool bMafiaWin)
{
    UE_LOG(LogTemp, Warning, TEXT("[GameState] 게임 종료 - %s 승리!"),
        bMafiaWin ? TEXT("마피아") : TEXT("시민팀"));
}

// 유언 알림 (모든 클라이언트)
void AMafiaGameState::MulticastNotifyLastWords_Implementation(const FString& PlayerId, const FString& Words)
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow,
            FString::Printf(TEXT("%s의 유언: %s"), *PlayerId, *Words));
    }

    UE_LOG(LogTemp, Warning, TEXT("플레이어 %s의 유언: %s"), *PlayerId, *Words);
}