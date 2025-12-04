#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "DedicatedCharacter.generated.h"

// 마피아 게임 플레이어 캐릭터 (네트워크 복제 지원)
UCLASS()
class ADedicatedCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    // 플레이어 이름 (네트워크 복제)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_PlayerName, Category = "Player Info")
    FString PlayerName;

    // 플레이어 이름 복제 시 호출되는 콜백
    UFUNCTION()
    void OnRep_PlayerName();

protected:
    UPROPERTY(Replicated)
    FString PlayerId; // 플레이어 고유 세션 토큰

    UPROPERTY(Replicated)
    FVector TargetLocation; // 목표 이동 위치

    UPROPERTY(Replicated)
    bool bIsMovingToTarget; // 이동 중 여부

    UPROPERTY(EditAnywhere, Category = "Movement")
    float MovementSpeed;

    // Enhanced Input 시스템
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    class UInputMappingContext* DefaultMappingContext;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    class UInputAction* SetDestinationClickAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    class UInputAction* SetDestinationTouchAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    class UNiagaraSystem* FXCursor;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    float ShortPressThreshold = 0.5f;

    // 플레이어 이름표 위젯
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
    class UWidgetComponent* NameplateWidget;

    class UUserWidget* CachedNameplateWidget;
    FTimerHandle JobsWidgetRetryTimer;
    int32 JobsWidgetRetryCount;
    FVector CachedDestination;
    float FollowTime;
    bool bIsTouch;

public:
    ADedicatedCharacter();

    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void Tick(float DeltaTime) override;
    virtual void BeginPlay() override;

    // 이름표 업데이트
    void UpdateNameplateWidgetForName();

    // 직업 정보 업데이트 (클라이언트 RPC)
    UFUNCTION(Client, Reliable)
    void UpdateNameplateWidgetForJobs(const FString& Jobs);

    // 화면 메시지 출력 (클라이언트 RPC)
    UFUNCTION(Client, Reliable)
    void ClientShowMessage(int key, float delay, FColor col, const FString& Text);

    // 입력 처리 함수들
    void OnInputStarted();
    void OnSetDestinationTriggered();
    void OnSetDestinationReleased();
    void OnTouchTriggered();
    void OnTouchReleased();
    bool GetClickedActor(AActor*& OutActor);

    FORCEINLINE FString GetPlayerId() const { return PlayerId; }

    UFUNCTION(BlueprintCallable)
    void SetPlayerId(const FString& Id) { PlayerId = Id; }

    UFUNCTION(BlueprintCallable, Category = "Player Info")
    FString GetPlayerName() const { return PlayerName; }

    // 서버에서 이동 처리
    UFUNCTION(Server, Reliable)
    void ServerMoveToLocation(FVector Location);

    void SetPlayerName(const FString& Name);
    void MoveToLocation(FVector Location);

    // 모든 클라이언트에 이동 명령 (멀티캐스트)
    UFUNCTION(NetMulticast, Reliable)
    void ForceMoveToLocation(FVector Location);

    // 밤 행동 요청 (서버 RPC)
    UFUNCTION(Server, Reliable)
    void ServerRequestNightAction(const FString& TargetId);

    // 투표 요청 (서버 RPC)
    UFUNCTION(Server, Reliable)
    void ServerRequestVote(const FString& TargetId);

    FORCEINLINE class UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent; }
    FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

    // 채팅 메시지 전송
    UFUNCTION(BlueprintCallable, Category = "Chat")
    void SendChatMessage(const FString& Message);

private:
    // 탑다운 뷰 카메라
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
    class UCameraComponent* TopDownCameraComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
    UCameraComponent* FollowCamera;

    // 카메라 붐
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
    class USpringArmComponent* CameraBoom;

    // 캐릭터 메시 컴포넌트들
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Mesh, meta = (AllowPrivateAccess = "true"))
    class USkeletalMeshComponent* SkeletalMeshComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Mesh, meta = (AllowPrivateAccess = "true"))
    class USkeletalMeshComponent* SkeletalMeshComponent1;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Mesh, meta = (AllowPrivateAccess = "true"))
    class USkeletalMeshComponent* SkeletalMeshComponent2;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Mesh, meta = (AllowPrivateAccess = "true"))
    class USkeletalMeshComponent* SkeletalMeshComponent3;
};