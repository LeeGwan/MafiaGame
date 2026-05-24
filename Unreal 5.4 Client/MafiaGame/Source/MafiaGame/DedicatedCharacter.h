// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "DedicatedCharacter.generated.h"

/**
 * @class ADedicatedCharacter
 * @brief Main player character class for the Mafia Game.
 * Implements network replication, top-down movement, and specialized game phase interactions.
 */
UCLASS()
class ADedicatedCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    /** Player display name - Replicated to all clients */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_PlayerName, Category = "Player Info")
    FString PlayerName;

    /** Callback triggered on clients when PlayerName is updated by the server */
    UFUNCTION()
    void OnRep_PlayerName();

protected:
    /** Unique session token for identifying the player across the network */
    UPROPERTY(Replicated)
    FString PlayerId;

    /** Destination for movement, synchronized across the network */
    UPROPERTY(Replicated)
    FVector TargetLocation;

    /** Movement state flag for network interpolation */
    UPROPERTY(Replicated)
    bool bIsMovingToTarget;

    /** Base movement speed of the character */
    UPROPERTY(EditAnywhere, Category = "Movement")
    float MovementSpeed;

    /** Enhanced Input: Mapping context for player controls */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    class UInputMappingContext* DefaultMappingContext;

    /** Enhanced Input: Action for destination selection/interaction */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    class UInputAction* SetDestinationClickAction;

    /** Enhanced Input: Action for mobile touch interaction */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    class UInputAction* SetDestinationTouchAction;

    /** Niagara FX spawned at the cursor destination */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    class UNiagaraSystem* FXCursor;

    /** Time threshold to distinguish between a short interaction and a long-press movement */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    float ShortPressThreshold = 0.5f;

    /** Screen-space widget component for player name and job display */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
    class UWidgetComponent* NameplateWidget;

    /** Cached pointer to the UI widget for optimized updates */
    class UUserWidget* CachedNameplateWidget;
    
    FTimerHandle JobsWidgetRetryTimer;
    int32 JobsWidgetRetryCount;
    FVector CachedDestination;
    float FollowTime;
    bool bIsTouch;

public:
    ADedicatedCharacter();

    // Overrides from ACharacter
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void Tick(float DeltaTime) override;
    virtual void BeginPlay() override;

    /** Updates the visual nameplate UI with the current PlayerName */
    void UpdateNameplateWidgetForName();

    /** [Client RPC] Updates the player's job information on the local UI */
    UFUNCTION(Client, Reliable)
    void UpdateNameplateWidgetForJobs(const FString& Jobs);

    /** [Client RPC] Displays a formatted message on the player's screen */
    UFUNCTION(Client, Reliable)
    void ClientShowMessage(int key, float delay, FColor col, const FString& Text);

    /** Input handling methods */
    void OnInputStarted();
    void OnSetDestinationTriggered();
    void OnSetDestinationReleased();
    void OnTouchTriggered();
    void OnTouchReleased();
    
    /** Performs a trace under the cursor to identify the actor being interacted with */
    bool GetClickedActor(AActor*& OutActor);

    /** Getters and Setters */
    FORCEINLINE FString GetPlayerId() const { return PlayerId; }

    UFUNCTION(BlueprintCallable)
    void SetPlayerId(const FString& Id) { PlayerId = Id; }

    UFUNCTION(BlueprintCallable, Category = "Player Info")
    FString GetPlayerName() const { return PlayerName; }

    /** [Server RPC] Validates and processes a movement request from the client */
    UFUNCTION(Server, Reliable)
    void ServerMoveToLocation(FVector Location);

    /** Sets the player name (Authority Only) */
    void SetPlayerName(const FString& Name);
    
    /** Initiates internal movement logic */
    void MoveToLocation(FVector Location);

    /** [NetMulticast RPC] Forcefully synchronizes character location across all clients */
    UFUNCTION(NetMulticast, Reliable)
    void ForceMoveToLocation(FVector Location);

    /** [Server RPC] Requests a specialized night-phase action (e.g., Kill, Investigate) */
    UFUNCTION(Server, Reliable)
    void ServerRequestNightAction(const FString& TargetId);

    /** [Server RPC] Casts a vote against a specific player during the voting phase */
    UFUNCTION(Server, Reliable)
    void ServerRequestVote(const FString& TargetId);

    /** Camera and Boom accessors */
    FORCEINLINE class UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent; }
    FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

    /** Broadcasts a chat message via the server */
    UFUNCTION(BlueprintCallable, Category = "Chat")
    void SendChatMessage(const FString& Message);

private:
    /** Main top-down perspective camera */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
    class UCameraComponent* TopDownCameraComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
    UCameraComponent* FollowCamera;

    /** Spring arm component for camera positioning and smoothing */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
    class USpringArmComponent* CameraBoom;

    /** Modular Skeletal Meshes for character customization (e.g., skins, equipment) */
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Mesh, meta = (AllowPrivateAccess = "true"))
    class USkeletalMeshComponent* SkeletalMeshComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Mesh, meta = (AllowPrivateAccess = "true"))
    class USkeletalMeshComponent* SkeletalMeshComponent1;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Mesh, meta = (AllowPrivateAccess = "true"))
    class USkeletalMeshComponent* SkeletalMeshComponent2;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Mesh, meta = (AllowPrivateAccess = "true"))
    class USkeletalMeshComponent* SkeletalMeshComponent3;
};
