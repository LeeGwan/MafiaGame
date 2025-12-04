#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "ServerConnector.h"
#include "DedicatedGameMode.generated.h"

class AJobs;
class ADirectionalLight;

// 마피아 게임 데디케이티드 서버 게임모드
UCLASS(minimalapi)
class ADedicatedGameMode : public AGameMode
{
    GENERATED_BODY()
protected:
    TMap<FString, FString> NightActions; // 밤 행동 저장 (PlayerId -> TargetId)
    TMap<FString, FString> VoteMap; // 투표 저장 (VoterId -> TargetId)
    TMap<FString, TPair<FString, FString>> TempPlayerDataMap; // 임시 플레이어 데이터 (Hash -> <Name, IP>)
    FString LastWords; // 유언
    TQueue<FVector> SpawnPosition; // 스폰 위치 큐
    TUniquePtr<ServerConnector> P_ServerConnector; // 게임 로비 서버 연결

protected:
    // 태양광 조명 (낮/밤 전환용)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting", meta = (AllowPrivateAccess = "true"))
    ADirectionalLight* SunLight;

    UPROPERTY()
    FRotator CurrentSunRotation;

    // 태양 회전 동기화 (멀티캐스트)
    UFUNCTION(NetMulticast, Reliable)
    void MulticastUpdateSunRotation(FRotator NewRotation);

    void SetDayTime(); // 낮 시간 설정
    void SetNightTime(); // 밤 시간 설정

    const int32 REQUIRED_PLAYERS = 6; // 게임 시작 필요 인원
    int32 ConnectedPlayers;

    // 페이즈 시간 설정 (테스트용 짧은 시간)
    const float NIGHT_DURATION = 30.0f;
    const float MORNING_DURATION = 30.0f;
    const float VOTING_DURATION = 30.0f;
    const float LASTWORDS_DURATION = 10.0f;
    /*
    const float NIGHT_DURATION = 120.0f;
    const float MORNING_DURATION = 300.0f;
    const float VOTING_DURATION = 120.0f;
    const float LASTWORDS_DURATION = 10.0f;*/

    FTimerHandle PhaseTimerHandle;
    FVector ExecutionSiteLocation; // 처형장 위치

    UPROPERTY()
    class AMafiaGameState* MafiaGameState;

public:
    ADedicatedGameMode();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // 로그인 전처리 (게임 로비 서버와 연동)
    virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

    // 로그인 후처리 (플레이어 초기화)
    virtual void PostLogin(APlayerController* NewPlayer) override;

    virtual void RestartPlayer(AController* NewPlayer) override;

    void FindPlayerStarts(); // 스폰 위치 찾기
    void InitPlayerInformaion(APlayerController* NewPlayer, const FString& UniqueIdStr);

    // 직업 할당 (마피아 2명, 경찰 1명, 탐정 1명, 시민 2명)
    UFUNCTION(BlueprintCallable)
    void AssignJobs();

    // 게임 시작 (6명 모두 접속 시)
    UFUNCTION(BlueprintCallable)
    void StartGame();

    // 페이즈 전환
    void StartNightPhase(); // 밤 페이즈 (마피아/경찰/탐정 행동)
    auto WaitPlayerPhase();
    void StartMorningPhase(); // 아침 페이즈 (밤 결과 공개)
    void StartVotingPhase(); // 투표 페이즈
    void StartLastWordsPhase(const FString& PlayerId); // 유언 페이즈
    void ProcessExecution(); // 처형 실행
    void OnPhaseTimeEnd();

    // 밤 행동 처리 (서버 RPC)
    UFUNCTION(Server, Reliable)
    void ServerProcessNightAction(const FString& PlayerId, const FString& TargetId);
    void ServerProcessNightAction_Implementation(const FString& PlayerId, const FString& TargetId);
    void ProcessNightResults(); // 밤 결과 처리

    // 투표 처리 (서버 RPC)
    UFUNCTION(Server, Reliable)
    void ServerCastVote(const FString& VoterId, const FString& TargetId);
    void ProcessVotingResults(); // 투표 결과 처리

    // 유언 제출 (서버 RPC)
    UFUNCTION(Server, Reliable)
    void ServerSubmitLastWords(const FString& PlayerId, const FString& Words);

    // 승리 조건 체크
    bool CheckWinCondition();

    class AMafiaPlayerState* FindPlayerStateByHash(const FString& PlayerHash);

    // 모든 플레이어를 시작 위치로 이동 (멀티캐스트)
    UFUNCTION(NetMulticast, Reliable)
    void MulticastMovePlayersToStart();

    // 이동 활성화/비활성화 (멀티캐스트)
    UFUNCTION(NetMulticast, Reliable)
    void MulticastSetMovementEnabled(bool bEnabled);

    // 메시지 출력
    void ALLUpdateMessage(int key, float delay, FColor col, const FString& Text);
    void UpdateMessage(class ADedicatedCharacter* Char, int key, float delay, FColor col, const FString& Text);

    // 투표 업데이트 알림 (멀티캐스트)
    UFUNCTION(NetMulticast, Reliable)
    void MulticastNotifyVoteUpdate(const FString& PlayerId, int32 NewVoteCount);

    // 채팅 메시지 전송 (서버 RPC)
    UFUNCTION(Server, Reliable)
    void ServerSendChatMessage(const FString& SenderHash, const FString& Message);
};