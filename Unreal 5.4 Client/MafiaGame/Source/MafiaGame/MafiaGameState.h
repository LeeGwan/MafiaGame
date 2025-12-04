// MafiaGameState.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "MafiaGameState.generated.h"

// 플레이어 스폰 위치 데이터
USTRUCT(BlueprintType)
struct FPlayerStartData
{
    GENERATED_BODY()

    UPROPERTY()
    FString PlayerHash; // 플레이어 세션 토큰

    UPROPERTY()
    FVector Location; // 스폰 위치

    FPlayerStartData()
        : PlayerHash(TEXT("")), Location(FVector::ZeroVector)
    {
    }

    FPlayerStartData(FString InHash, FVector InLocation)
        : PlayerHash(InHash), Location(InLocation)
    {
    }
};

// 게임 페이즈 (네트워크 복제)
UENUM(BlueprintType)
enum class EGamePhase : uint8
{
    Waiting,    // 대기 중
    Night,      // 밤 (마피아/경찰/탐정 행동)
    Morning,    // 아침 (결과 공개)
    Voting,     // 투표
    LastWords,  // 유언
    GameOver    // 게임 종료
};

// 마피아 게임 상태 (모든 클라이언트와 동기화)
UCLASS()
class MAFIAGAME_API AMafiaGameState : public AGameState
{
    GENERATED_BODY()

protected:
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
    EGamePhase CurrentPhase; // 현재 페이즈

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
    float PhaseTimer; // 페이즈 타이머

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
    int32 DayCount; // 현재 날짜

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
    int32 MafiaCount; // 마피아 생존자 수

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
    int32 CitizenTeamCount; // 시민팀 생존자 수

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
    int32 AlivePlayerCount; // 총 생존자 수

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
    FString ExecutedPlayerId; // 처형된 플레이어 ID

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
    TArray<FString> PlayerHashes; // 모든 플레이어 세션 토큰

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
    TArray<FPlayerStartData> PlayerStartLocations; // 플레이어별 스폰 위치

public:
    AMafiaGameState();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Getters
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

    // 플레이어 세션 토큰으로 스폰 위치 찾기
    UFUNCTION(BlueprintCallable, Category = "Game State")
    FVector GetPlayerStartLocationByHash(const FString& PlayerHash) const;

    // Setters (서버 전용)
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

    // 페이즈 변경 알림 (멀티캐스트)
    UFUNCTION(NetMulticast, Reliable)
    void MulticastPhaseChanged(EGamePhase NewPhase, float Duration);

    // 게임 종료 알림 (멀티캐스트)
    UFUNCTION(NetMulticast, Reliable)
    void MulticastGameOver(bool bMafiaWin);

    // 플레이어 사망 알림 (멀티캐스트)
    UFUNCTION(NetMulticast, Reliable)
    void MulticastNotifyPlayerDeath(const FString& PlayerId, const FString& PlayerName);

    // 유언 알림 (멀티캐스트)
    UFUNCTION(NetMulticast, Reliable)
    void MulticastNotifyLastWords(const FString& PlayerId, const FString& Words);
};