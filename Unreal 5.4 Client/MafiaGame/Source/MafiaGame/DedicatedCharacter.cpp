// Copyright Epic Games, Inc. All Rights Reserved.
// Implementation of the Mafia Game Player Character

#include "DedicatedCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Misc/CommandLine.h"
#include "Components/DecalComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/TextBlock.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/Material.h"
#include "Engine/World.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "DedicatedGameMode.h"
#include "MafiaGameState.h"
#include "MafiaPlayerState.h"

/**
 * @brief Constructor: Sets up default components, movement properties, and network replication.
 */
ADedicatedCharacter::ADedicatedCharacter()
{
    PrimaryActorTick.bStartWithTickEnabled = true;
    PrimaryActorTick.bCanEverTick = true;

    // Initialize default properties
    PlayerId = TEXT("");
    PlayerName = TEXT("123");
    bIsMovingToTarget = false;
    MovementSpeed = 500.0f;
    CachedDestination = FVector::ZeroVector;
    FollowTime = 0.f;
    JobsWidgetRetryCount = 0;
    bIsTouch = false;
    CachedNameplateWidget = nullptr;
    NameplateWidget = nullptr;

    // Set size for collision capsule
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    // Setup modular skeletal meshes attached to the main mesh
    SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComp"));
    SkeletalMeshComponent->SetupAttachment(GetMesh());

    SkeletalMeshComponent1 = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComp1"));
    SkeletalMeshComponent1->SetupAttachment(GetMesh());

    SkeletalMeshComponent2 = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComp2"));
    SkeletalMeshComponent2->SetupAttachment(GetMesh());

    SkeletalMeshComponent3 = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComp3"));
    SkeletalMeshComponent3->SetupAttachment(GetMesh());

    // Character rotation settings for top-down view
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    // Configure character movement settings
    if (GetCharacterMovement())
    {
        GetCharacterMovement()->MaxWalkSpeed = MovementSpeed;
        GetCharacterMovement()->bOrientRotationToMovement = true;
        GetCharacterMovement()->RotationRate = FRotator(0.0f, 640.0f, 0.0f);
        GetCharacterMovement()->bUseControllerDesiredRotation = false;
        
        // Smooth network movement
        GetCharacterMovement()->NetworkSmoothingMode = ENetworkSmoothingMode::Exponential;
        GetCharacterMovement()->NetworkMaxSmoothUpdateDistance = 92.f;
        GetCharacterMovement()->NetworkNoSmoothUpdateDistance = 140.f;
    }

    // Enable Network Replication
    bReplicates = true;
    SetReplicateMovement(true);

    // Camera Boom for top-down perspective
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->SetUsingAbsoluteRotation(true);
    CameraBoom->TargetArmLength = 1800.f;
    CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
    CameraBoom->bDoCollisionTest = false;

    // Top-down camera component
    TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
    TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    TopDownCameraComponent->bUsePawnControlRotation = false;

    // Nameplate UI component attached to the character's head
    NameplateWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("NameplateWidget"));
    NameplateWidget->SetupAttachment(GetMesh());
    NameplateWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 190.0f));
    NameplateWidget->SetWidgetSpace(EWidgetSpace::Screen);
    NameplateWidget->SetDrawSize(FVector2D(200.0f, 50.0f));
}

/**
 * @brief Registers properties for network replication across server and clients.
 */
void ADedicatedCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ADedicatedCharacter, PlayerId);
    DOREPLIFETIME(ADedicatedCharacter, PlayerName);
    DOREPLIFETIME(ADedicatedCharacter, TargetLocation);
    DOREPLIFETIME(ADedicatedCharacter, bIsMovingToTarget);
}

/**
 * @brief Frame-by-frame update: Handles movement interpolation towards the target location.
 */
void ADedicatedCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bIsMovingToTarget)
    {
        FVector CurrentLocation = GetActorLocation();
        FVector Direction = (TargetLocation - CurrentLocation).GetSafeNormal();
        float Distance = FVector::Dist(CurrentLocation, TargetLocation);

        if (Distance > 50.0f)
        {
            // Rotate towards destination and apply movement
            FRotator TargetRotation = UKismetMathLibrary::FindLookAtRotation(CurrentLocation, TargetLocation);
            SetActorRotation(FRotator(0.0f, TargetRotation.Yaw, 0.0f));
            AddMovementInput(Direction, 1.0f);
        }
        else
        {
            bIsMovingToTarget = false;
        }
    }
}

/**
 * @brief Game start initialization: Mesh syncing, Input mode configuration, and UI Loading.
 */
void ADedicatedCharacter::BeginPlay()
{
    Super::BeginPlay();

    // Sync skeletal mesh poses for modular cosmetics
    if (GetMesh())
    {
        SkeletalMeshComponent->SetLeaderPoseComponent(GetMesh());
        SkeletalMeshComponent1->SetLeaderPoseComponent(GetMesh());
        SkeletalMeshComponent2->SetLeaderPoseComponent(GetMesh());
        SkeletalMeshComponent3->SetLeaderPoseComponent(GetMesh());
    }

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (PC)
    {
        // UI Interaction settings
        PC->bShowMouseCursor = true;
        PC->bEnableClickEvents = true;
        PC->bEnableMouseOverEvents = true;
        FInputModeGameAndUI InputMode;
        InputMode.SetHideCursorDuringCapture(false);
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(InputMode);

        // Update character visibility through PlayerState
        if (AMafiaPlayerState* MyPS = Cast<AMafiaPlayerState>(PC->PlayerState))
        {
            TWeakObjectPtr<AMafiaPlayerState> WeakPS(MyPS);
            FTimerHandle VisibilityTimer;
            GetWorld()->GetTimerManager().SetTimer(VisibilityTimer, [WeakPS]()
                {
                    if (WeakPS.IsValid())
                    {
                        WeakPS->UpdateAllCharacterVisibility();
                    }
                }, 0.5f, false);
        }
    }

    // Async Loading for Player Nameplate UI to optimize performance
    if (NameplateWidget)
    {
        FSoftClassPath WidgetClassPath(TEXT("/Game/TopDown/Blueprints/WBP_PlayerNameplate.WBP_PlayerNameplate_C"));
        UClass* WidgetClass = WidgetClassPath.TryLoadClass<UUserWidget>();

        if (WidgetClass)
        {
            NameplateWidget->SetWidgetClass(WidgetClass);
            NameplateWidget->InitWidget();
            UpdateNameplateWidgetForName();
        }
    }
}

/**
 * @brief Updates the player's name text in the screen-space widget.
 */
void ADedicatedCharacter::UpdateNameplateWidgetForName()
{
    if (!NameplateWidget || !NameplateWidget->GetWidget()) return;

    CachedNameplateWidget = Cast<UUserWidget>(NameplateWidget->GetWidget());
    if (CachedNameplateWidget)
    {
        UTextBlock* NameText = Cast<UTextBlock>(CachedNameplateWidget->GetWidgetFromName(TEXT("PlayerNameText")));
        if (NameText)
        {
            NameText->SetText(FText::FromString(PlayerName));
        }
    }
}

/**
 * @brief [Client RPC] Updates the job title text in the player's nameplate.
 */
void ADedicatedCharacter::UpdateNameplateWidgetForJobs_Implementation(const FString& Jobs)
{
    if (CachedNameplateWidget)
    {
        UTextBlock* NameText = Cast<UTextBlock>(CachedNameplateWidget->GetWidgetFromName(TEXT("PlayerJobText")));
        if (NameText)
        {
            NameText->SetText(FText::FromString(Jobs));
        }
    }
}

/**
 * @brief Configures Enhanced Input bindings for character interaction.
 */
void ADedicatedCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            if (DefaultMappingContext)
            {
                Subsystem->AddMappingContext(DefaultMappingContext, 0);
            }
        }
    }

    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        if (SetDestinationClickAction)
        {
            EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Started, this, &ADedicatedCharacter::OnInputStarted);
            EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Triggered, this, &ADedicatedCharacter::OnSetDestinationTriggered);
            EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Completed, this, &ADedicatedCharacter::OnSetDestinationReleased);
            EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Canceled, this, &ADedicatedCharacter::OnSetDestinationReleased);
        }
    }
}

/**
 * @brief [Client RPC] Displays an on-screen debug message for the player.
 */
void ADedicatedCharacter::ClientShowMessage_Implementation(int key, float delay, FColor col, const FString& Text)
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(key, delay, col, Text);
    }
}

void ADedicatedCharacter::OnInputStarted()
{
    // Implementation for input start event
}

/**
 * @brief Calculates destination based on cursor/touch location and applies movement.
 */
void ADedicatedCharacter::OnSetDestinationTriggered()
{
    FollowTime += GetWorld()->GetDeltaSeconds();

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    FHitResult Hit;
    bool bHitSuccessful = bIsTouch ? 
        PC->GetHitResultUnderFinger(ETouchIndex::Touch1, ECC_Visibility, true, Hit) : 
        PC->GetHitResultUnderCursor(ECC_Visibility, true, Hit);

    if (bHitSuccessful)
    {
        CachedDestination = Hit.Location;
    }

    FVector WorldDirection = (CachedDestination - GetActorLocation()).GetSafeNormal();
    AddMovementInput(WorldDirection, 1.0, false);
}

/**
 * @brief Logic for input release: Differentiates between interaction (Short Press) and movement (Long Press).
 */
void ADedicatedCharacter::OnSetDestinationReleased()
{
    AMafiaGameState* GameState = Cast<AMafiaGameState>(GetWorld()->GetGameState());

    if (FollowTime <= ShortPressThreshold)
    {
        APlayerController* PC = Cast<APlayerController>(GetController());
        if (!PC) return;

        AMafiaPlayerState* MyPS = Cast<AMafiaPlayerState>(PC->PlayerState);

        if (GameState && MyPS && MyPS->GetIsAlive())
        {
            AActor* ClickedActor = nullptr;
            if (GetClickedActor(ClickedActor))
            {
                ADedicatedCharacter* ClickedCharacter = Cast<ADedicatedCharacter>(ClickedActor);
                if (ClickedCharacter)
                {
                    FString ClickedPlayerId = ClickedCharacter->GetPlayerId();
                    if (ClickedPlayerId.IsEmpty()) return;

                    // Execute Phase-specific actions (Vote or Night Ability)
                    if (GameState->GetCurrentPhase() == EGamePhase::Night)
                    {
                        if (MyPS->GetPlayerHash() != ClickedPlayerId)
                        {
                            ServerRequestNightAction(ClickedPlayerId);
                        }
                    }
                    else if (GameState->GetCurrentPhase() == EGamePhase::Voting)
                    {
                        ServerRequestVote(ClickedPlayerId);
                    }
                    return;
                }
            }

            // Fallback: Just move to location if no actor was clicked
            UAIBlueprintHelperLibrary::SimpleMoveToLocation(PC, CachedDestination);
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, FXCursor, CachedDestination,
                FRotator::ZeroRotator, FVector(1.f, 1.f, 1.f), true, true, ENCPoolMethod::None, true);
        }
    }
    FollowTime = 0.f;
}

void ADedicatedCharacter::OnTouchTriggered()
{
    bIsTouch = true;
    OnSetDestinationTriggered();
}

void ADedicatedCharacter::OnTouchReleased()
{
    bIsTouch = false;
    OnSetDestinationReleased();
}

/**
 * @brief Traces for an actor under the cursor for selection logic.
 */
bool ADedicatedCharacter::GetClickedActor(AActor*& OutActor)
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return false;

    FHitResult HitResult;
    bool bHit = PC->GetHitResultUnderCursor(ECC_Pawn, false, HitResult);

    if (bHit && HitResult.GetActor())
    {
        OutActor = HitResult.GetActor();
        return true;
    }
    return false;
}

/**
 * @brief Callback for Replicated PlayerName: Updates UI on clients when name changes.
 */
void ADedicatedCharacter::OnRep_PlayerName()
{
    UpdateNameplateWidgetForName();
}

/**
 * @brief Sets player name (Server Only).
 */
void ADedicatedCharacter::SetPlayerName(const FString& Name)
{
    if (HasAuthority())
    {
        PlayerName = Name;
        UpdateNameplateWidgetForName();
    }
}

/**
 * @brief Triggers internal movement logic to a target location.
 */
void ADedicatedCharacter::MoveToLocation(FVector Location)
{
    TargetLocation = Location;
    bIsMovingToTarget = true;
}

/**
 * @brief [Server RPC] Casts a vote against a specific target player.
 */
void ADedicatedCharacter::ServerRequestVote_Implementation(const FString& TargetId)
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    AMafiaPlayerState* MyPS = Cast<AMafiaPlayerState>(PC->PlayerState);
    ADedicatedGameMode* GameMode = Cast<ADedicatedGameMode>(GetWorld()->GetAuthGameMode());

    if (GameMode && MyPS)
    {
        GameMode->ServerCastVote(MyPS->GetPlayerHash(), TargetId);
    }
}

/**
 * @brief [Server RPC] Executes specific night actions (Kill, Investigate, Protect, etc.).
 */
void ADedicatedCharacter::ServerRequestNightAction_Implementation(const FString& TargetId)
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    AMafiaPlayerState* MyPS = Cast<AMafiaPlayerState>(PC->PlayerState);
    if (!MyPS || !MyPS->GetIsAlive() || !MyPS->CanNightAction()) return;

    ADedicatedGameMode* GameMode = Cast<ADedicatedGameMode>(GetWorld()->GetAuthGameMode());
    if (GameMode)
    {
        GameMode->ServerProcessNightAction(MyPS->GetPlayerHash(), TargetId);
    }
}

/**
 * @brief Forwards a chat message to the server for distribution.
 */
void ADedicatedCharacter::SendChatMessage(const FString& Message)
{
    if (Message.IsEmpty()) return;

    APlayerController* PC = Cast<APlayerController>(GetController());
    AMafiaPlayerState* MyPS = Cast<AMafiaPlayerState>(PC->PlayerState);
    ADedicatedGameMode* GameMode = Cast<ADedicatedGameMode>(GetWorld()->GetAuthGameMode());

    if (GameMode && MyPS)
    {
        GameMode->ServerSendChatMessage(MyPS->GetPlayerHash(), Message);
    }
}

/**
 * @brief [Multicast RPC] Forcefully teleports the character to a location.
 */
void ADedicatedCharacter::ForceMoveToLocation_Implementation(FVector Location)
{
    SetActorLocation(Location);
    TargetLocation = Location;
    bIsMovingToTarget = false;
}

/**
 * @brief [Server RPC] Syncs character location to the server.
 */
void ADedicatedCharacter::ServerMoveToLocation_Implementation(FVector Location)
{
    SetActorLocation(Location);
}
