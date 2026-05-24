// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "MafiaPlayerState.generated.h"

/**
 * @enum EJobType
 * @brief Defines specialized roles within the Mafia Game.
 */
UENUM(BlueprintType)
enum class EJobType : uint8
{
    None,       // Unassigned state
    Mafia,      // Faction: Mafia - Objective: Eliminate citizens at night
    Police,     // Faction: Citizen - Ability: Investigate a player to reveal if they are Mafia
    Detective,  // Faction: Citizen - Ability: Surveillance (Tracks who a player visited at night)
    Citizen     // Faction: Citizen - No special abilities
};

/**
 * @class AMafiaPlayerState
 * @brief Manages synchronized player data, role status, and action outcomes.
 * Handles network replication for gameplay-critical information within the MafiaGame framework.
 */
UCLASS()
class MAFIAGAME_API AMafiaPlayerState : public APlayerState
{
    GENERATED_BODY()

protected:
    /** Unique session hash used for cross-session player identification */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player State")
    FString PlayerHash;

    /** The specific job role assigned to this player */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player State")
    EJobType JobType;

    /** Survival status; triggers OnRep_IsAlive on clients when changed */
    UPROPERTY(ReplicatedUsing = OnRep_IsAlive, BlueprintReadOnly, Category = "Player State")
    bool bIsAlive;

    /** Total number of votes received during the voting phase */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player State")
    int32 VoteCount;

    /** The session ID of the player targeted by this player's night ability */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player State")
    FString TargetPlayerId;

    /** The designated spawn coordinates assigned by the server */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player State")
    FVector SpawnPosition;

    /** [Police Only] Cached result of the last investigation (True if target is Mafia) */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player State")
    bool bInvestigationResult;

    /** [Detective Only] surveillance data revealing the target's night-time movement/visit */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player State")
    FString VisitInfo;

    /** The player's display name, synchronized via network */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player State")
    FString PlayerNickName;

public:
    AMafiaPlayerState();

    /** Registers properties for network synchronization (DOREPLIFETIME) */
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** Getters */
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

    /** Returns the readable string representation of the job name */
    UFUNCTION(BlueprintCallable, Category = "Player State")
    FString GetJobName() const;

    /** Validates if the player's role is capable of performing actions during the night phase */
    UFUNCTION(BlueprintCallable, Category = "Player State")
    bool CanNightAction();

    /** Setters (Strictly Authority/Server-Only) */
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

    /** Client RPCs - Notification Handlers */
    
    /** [Client RPC] Notifies the player of their assigned role at start of session */
    UFUNCTION(Client, Reliable)
    void ClientNotifyJobAssigned(EJobType AssignedJob);

    /** [Client RPC] Delivers private investigation results to the Police player */
    UFUNCTION(Client, Reliable)
    void ClientNotifyInvestigationResult(const FString& TargetId, bool bIsMafia);

    /** [Client RPC] Delivers surveillance/visit data to the Detective player */
    UFUNCTION(Client, Reliable)
    void ClientNotifyVisitInfo(const FString& Info);

    /** Replication Notify: Triggered on clients when the survival status changes */
    UFUNCTION()
    void OnRep_IsAlive();

    /** * @brief Refreshes actor visibility for "Ghost Mode".
     * Living players cannot see the deceased; dead players can spectate all.
     */
    void UpdateAllCharacterVisibility();

    /** [Client RPC] Processes received chat messages and routes them to the local UI */
    UFUNCTION(Client, Reliable)
    void ClientReceiveChatMessage(const FString& SenderHash, const FString& SenderName, const FString& Message, bool bSenderIsDead, FLinearColor MessageColor);

    /** Delegate for chat events; allows Blueprint UI to bind to incoming messages */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnChatMessageReceived, FString, SenderHash, FString, SenderName, FString, Message, bool, bSenderIsDead, FLinearColor, MessageColor);

    UPROPERTY(BlueprintAssignable, Category = "Chat")
    FOnChatMessageReceived OnChatMessageReceived;
};
