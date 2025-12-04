// MafiaPlayerState.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "MafiaPlayerState.generated.h"

// 직업 타입
UENUM(BlueprintType)
enum class EJobType : uint8
{
    None,       // 미할당
    Mafia,      // 마피아 (밤에 시민 죽임)
    Police,     // 경찰 (밤에 마피아 조사)
    Detective,  // 탐정 (밤에 방문자 조사)
    Citizen     // 시민 (특수 능력 없음)
};

// 마피아 게임 플레이어 상태 (네트워크 복제)
UCLASS()
class MAFIAGAME_API AMafiaPlayerState : public APlayerState
{
    GENERATED_BODY()

protected:
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player State")
    FString PlayerHash; // 플레이어 세션 토큰

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player State")
    EJobType JobType; // 할당된 직업

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player State")
    bool bIsAlive; // 생존 여부

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player State")
    int32 VoteCount; // 받은 투표 수

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player State")
    FString TargetPlayerId; // 밤 행동 대상 ID

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player State")
    FVector SpawnPosition; // 스폰 위치

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player State")
    bool bInvestigationResult; // 경찰 조사 결과 (true: 마피아)

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player State")
    FString VisitInfo; // 탐정 조사 결과 (방문자 정보)

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player State")
    FString PlayerNickName; // 플레이어 닉네임

public:
    AMafiaPlayerState();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Getters
    UFUNCTION(BlueprintCallable, Category = "Player State")
    FVector GetSpawnPosition() const { return SpawnPosition; }

    UFUNCTION(BlueprintCallable, Category = "Player State")
    FString GetNickName() const { return PlayerNickName; }

    UFUNCTION(BlueprintCallable, Category = "Player State")
    FString GetPlayerHash() const { return PlayerHash; }

    UFUNCTION(BlueprintCallable, Category = "Player State")
    EJobType GetJobType() const { return JobType; }

    UFUNCTION(BlueprintCallable, Category = "Player State")
    bool GetIsAlive() const { return bIsAlive; }

    UFUNCTION(BlueprintCallable, Category = "Player State")
    int32 GetVoteCount() const { return VoteCount; }

    UFUNCTION(BlueprintCallable, Category = "Player State")
    FString GetTargetPlayerId() const { return TargetPlayerId; }

    UFUNCTION(BlueprintCallable, Category = "Player State")
    FString GetJobName() const;

    // 밤 행동 가능 여부 확인
    UFUNCTION(BlueprintCallable, Category = "Player State")
    bool CanNightAction();

    // Setters (서버 전용)
    void SetSpawnPosition(const FVector& POS);
    void SetPlayerHash(const FString& Hash);
    void SetNickName(const FString& NickName);
    void SetJobType(EJobType NewJobType);
    void SetAlive(bool bAlive);
    void SetVoteCount(int32 Count);
    void AddVote();
    void ResetVotes();
    void SetTarget(const FString& TargetId);
    void ResetTarget();
    void SetInvestigationResult(bool bIsMafia);
    void SetVisitInfo(const FString& Info);

    // 클라이언트 RPC
    UFUNCTION(Client, Reliable)
    void ClientNotifyJobAssigned(EJobType AssignedJob);

    UFUNCTION(Client, Reliable)
    void ClientNotifyInvestigationResult(const FString& TargetId, bool bIsMafia);

    UFUNCTION(Client, Reliable)
    void ClientNotifyVisitInfo(const FString& Info);

    // 생존 상태 변경 시 호출
    UFUNCTION()
    void OnRep_IsAlive();

    // 모든 캐릭터 가시성 업데이트
    void UpdateAllCharacterVisibility();

    // 채팅 메시지 수신 (클라이언트 RPC)
    UFUNCTION(Client, Reliable)
    void ClientReceiveChatMessage(const FString& SenderHash, const FString& SenderName, const FString& Message, bool bSenderIsDead, FLinearColor MessageColor);

    // 채팅 메시지 수신 델리게이트
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnChatMessageReceived, FString, SenderHash, FString, SenderName, FString, Message, bool, bSenderIsDead, FLinearColor, MessageColor);

    UPROPERTY(BlueprintAssignable, Category = "Chat")
    FOnChatMessageReceived OnChatMessageReceived;
};