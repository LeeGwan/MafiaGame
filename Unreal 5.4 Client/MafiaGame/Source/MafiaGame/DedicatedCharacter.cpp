// Copyright Epic Games, Inc. All Rights Reserved.
// 마피아 게임 플레이어 캐릭터 구현

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
#include "GameFramework/PlayerController.h"

// 캐릭터 초기화 (컴포넌트 설정, 네트워크 복제 설정)
ADedicatedCharacter::ADedicatedCharacter()
{
    PrimaryActorTick.bStartWithTickEnabled = true;
    PrimaryActorTick.bCanEverTick = true;
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
    // Capsule이 Root (Character 기본 구조)
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    // SkeletalMesh들도 Capsule 아래에 붙임
    SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComp"));
    SkeletalMeshComponent->SetupAttachment(GetMesh());

    SkeletalMeshComponent1 = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComp1"));
    SkeletalMeshComponent1->SetupAttachment(GetMesh());

    SkeletalMeshComponent2 = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComp2"));
    SkeletalMeshComponent2->SetupAttachment(GetMesh());

    SkeletalMeshComponent3 = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComp3"));
    SkeletalMeshComponent3->SetupAttachment(GetMesh());

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    if (GetCharacterMovement())
    {
        GetCharacterMovement()->MaxWalkSpeed = MovementSpeed;
        GetCharacterMovement()->bOrientRotationToMovement = true;
        GetCharacterMovement()->RotationRate = FRotator(0.0f, 640.0f, 0.0f);
        GetCharacterMovement()->bUseControllerDesiredRotation = false;
        GetCharacterMovement()->NetworkSmoothingMode = ENetworkSmoothingMode::Exponential;
        GetCharacterMovement()->NetworkMaxSmoothUpdateDistance = 92.f;
        GetCharacterMovement()->NetworkNoSmoothUpdateDistance = 140.f;
    }

    bReplicates = true;
    SetReplicateMovement(true);

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->SetUsingAbsoluteRotation(true);
    CameraBoom->TargetArmLength = 1800.f;
    CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
    CameraBoom->bDoCollisionTest = false;

    TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
    TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    TopDownCameraComponent->bUsePawnControlRotation = false;

    // NameplateWidget 생성
    NameplateWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("NameplateWidget"));
    NameplateWidget->SetupAttachment(GetMesh());
    NameplateWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 190.0f));
    NameplateWidget->SetWidgetSpace(EWidgetSpace::Screen);
    NameplateWidget->SetDrawSize(FVector2D(200.0f, 50.0f));
}

// 네트워크 복제 속성 등록 (PlayerId, PlayerName, 이동 정보)
void ADedicatedCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ADedicatedCharacter, PlayerId);
    DOREPLIFETIME(ADedicatedCharacter, PlayerName);
    DOREPLIFETIME(ADedicatedCharacter, TargetLocation);
    DOREPLIFETIME(ADedicatedCharacter, bIsMovingToTarget);
}

// 매 프레임 호출 (타겟 위치로 이동 처리)
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

// 게임 시작 시 초기화 (메시 설정, 입력 설정, 이름표 위젯 로드)
void ADedicatedCharacter::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("[Character] BeginPlay Start - %s"), *GetName());

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
        PC->bShowMouseCursor = true;
        PC->bEnableClickEvents = true;
        PC->bEnableMouseOverEvents = true;
        FInputModeGameAndUI InputMode;
        InputMode.SetHideCursorDuringCapture(false);
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(InputMode);

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


    if (NameplateWidget && NameplateWidget->GetWidget())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Character] Loading Widget Async..."));

        // 비동기 로딩 방식으로 변경
        FSoftClassPath WidgetClassPath(TEXT("/Game/TopDown/Blueprints/WBP_PlayerNameplate.WBP_PlayerNameplate_C"));
        UClass* WidgetClass = WidgetClassPath.TryLoadClass<UUserWidget>();

        if (WidgetClass)
        {
            NameplateWidget->SetWidgetClass(WidgetClass);
            NameplateWidget->InitWidget();
            UpdateNameplateWidgetForName();
            UE_LOG(LogTemp, Warning, TEXT("[Character] Widget Loaded Successfully"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[Character] Failed to load widget class!"));
        }
    }
    if (!NameplateWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("NameplateWidget!!"));
    }
    if (!NameplateWidget->GetWidget())
    {
        UE_LOG(LogTemp, Error, TEXT("NameplateWidget   GetWidget!!"));
    }
    UE_LOG(LogTemp, Warning, TEXT("[Character] BeginPlay End - %s"), *GetName());
}

// 이름표 위젯에 플레이어 이름 업데이트
void ADedicatedCharacter::UpdateNameplateWidgetForName()
{
    APlayerController* PC = Cast<APlayerController>(GetController());


    if (!NameplateWidget || !NameplateWidget->GetWidget())
    {

        return;
    }

    CachedNameplateWidget = Cast<UUserWidget>(NameplateWidget->GetWidget());
    if (CachedNameplateWidget)
    {
        UTextBlock* NameText = Cast<UTextBlock>(CachedNameplateWidget->GetWidgetFromName(TEXT("PlayerNameText")));
        if (NameText)
        {
            NameText->SetText(FText::FromString(PlayerName));

        }
        else
        {
            GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("PlayerNameText 못 찾음!"));
        }
    }
}

// 이름표 위젯에 직업 정보 업데이트 (클라이언트 RPC)
void ADedicatedCharacter::UpdateNameplateWidgetForJobs_Implementation(const FString& Jobs)
{


    if (CachedNameplateWidget)
    {
        UTextBlock* NameText = Cast<UTextBlock>(CachedNameplateWidget->GetWidgetFromName(TEXT("PlayerJobText")));
        if (NameText)
        {
            NameText->SetText(FText::FromString(Jobs));
        }
        else
        {
            GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("PlayerJobText 못 찾음!"));
        }
    }

}


// 입력 컴포넌트 설정 (Enhanced Input 시스템)
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
            EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Started,
                this, &ADedicatedCharacter::OnInputStarted);
            EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Triggered,
                this, &ADedicatedCharacter::OnSetDestinationTriggered);
            EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Completed,
                this, &ADedicatedCharacter::OnSetDestinationReleased);
            EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Canceled,
                this, &ADedicatedCharacter::OnSetDestinationReleased);
        }
    }
}


// 화면에 메시지 출력 (클라이언트 RPC)
void ADedicatedCharacter::ClientShowMessage_Implementation(int key, float delay, FColor col, const FString& Text)
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(key, delay, col, Text);
    }
}

// 입력 시작 이벤트
void ADedicatedCharacter::OnInputStarted()
{

}

// 목표 지점 설정 트리거 (마우스/터치로 이동 방향 계산)
void ADedicatedCharacter::OnSetDestinationTriggered()
{

    FollowTime += GetWorld()->GetDeltaSeconds();

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    FHitResult Hit;
    bool bHitSuccessful = false;

    if (bIsTouch)
    {
        bHitSuccessful = PC->GetHitResultUnderFinger(ETouchIndex::Touch1,
            ECollisionChannel::ECC_Visibility, true, Hit);
    }
    else
    {
        bHitSuccessful = PC->GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, Hit);
    }

    if (bHitSuccessful)
    {
        CachedDestination = Hit.Location;

    }


    FVector WorldDirection = (CachedDestination - GetActorLocation()).GetSafeNormal();
    AddMovementInput(WorldDirection, 1.0, false);
}

// 마우스/터치 릴리즈 (짧은 클릭: 플레이어 선택 또는 이동, 긴 클릭: 이동만)
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

                    if (ClickedPlayerId.IsEmpty())
                    {

                        return;
                    }

                    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
                        FString::Printf(TEXT("[Input] 클릭한 플레이어: %s"), *ClickedPlayerId));

                    if (GameState->GetCurrentPhase() == EGamePhase::Night)
                    {

                        FString MyPlayerId = MyPS->GetPlayerHash();
                        if (MyPlayerId != ClickedPlayerId)
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


            UAIBlueprintHelperLibrary::SimpleMoveToLocation(PC, CachedDestination);
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, FXCursor, CachedDestination,
                FRotator::ZeroRotator, FVector(1.f, 1.f, 1.f), true, true, ENCPoolMethod::None, true);
        }
        else
        {


            UAIBlueprintHelperLibrary::SimpleMoveToLocation(PC, CachedDestination);
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, FXCursor, CachedDestination,
                FRotator::ZeroRotator, FVector(1.f, 1.f, 1.f), true, true, ENCPoolMethod::None, true);
        }
    }


    FollowTime = 0.f;
}

// 터치 입력 시작
void ADedicatedCharacter::OnTouchTriggered()
{
    bIsTouch = true;
    OnSetDestinationTriggered();
}

// 터치 입력 종료
void ADedicatedCharacter::OnTouchReleased()
{
    bIsTouch = false;
    OnSetDestinationReleased();
}

// 커서 위치의 액터 가져오기 (플레이어 선택용)
bool ADedicatedCharacter::GetClickedActor(AActor*& OutActor)
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC)
    {

        return false;
    }

    FHitResult HitResult;
    bool bHit = PC->GetHitResultUnderCursor(ECC_Pawn, false, HitResult);



    if (bHit && HitResult.GetActor())
    {
        OutActor = HitResult.GetActor();

        return true;
    }

    return false;
}

// 플레이어 이름 복제 콜백
void ADedicatedCharacter::OnRep_PlayerName()
{
    UpdateNameplateWidgetForName();
}

// 플레이어 이름 설정 (서버 전용)
void ADedicatedCharacter::SetPlayerName(const FString& Name)
{
    if (HasAuthority())
    {
        PlayerName = Name;
        UpdateNameplateWidgetForName();

    }
}

// 목표 위치로 이동 시작
void ADedicatedCharacter::MoveToLocation(FVector Location)
{
    TargetLocation = Location;
    bIsMovingToTarget = true;
}


// 투표 요청 (서버 RPC)
void ADedicatedCharacter::ServerRequestVote_Implementation(const FString& TargetId)
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    AMafiaPlayerState* MyPS = Cast<AMafiaPlayerState>(PC->PlayerState);
    if (!MyPS) return;

    ADedicatedGameMode* GameMode = Cast<ADedicatedGameMode>(GetWorld()->GetAuthGameMode());
    if (GameMode)
    {
        FString VoterId = MyPS->GetPlayerHash();
        GameMode->ServerCastVote(VoterId, TargetId);
    }
}

// 밤 행동 요청 (서버 RPC: 마피아/경찰/탐정)
void ADedicatedCharacter::ServerRequestNightAction_Implementation(const FString& TargetId)
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    AMafiaPlayerState* MyPS = Cast<AMafiaPlayerState>(PC->PlayerState);
    if (!MyPS) return;

    if (!MyPS->GetIsAlive())
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red,
            TEXT("당신은 사망 하였습니다"));
    }
    if (!MyPS->CanNightAction())
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red,
            TEXT("당신은 특수 직업이 아닙니다"));
    }
    ADedicatedGameMode* GameMode = Cast<ADedicatedGameMode>(GetWorld()->GetAuthGameMode());
    if (GameMode)
    {
        FString MyPlayerId = MyPS->GetPlayerHash();
        if (MyPlayerId != TargetId)
        {
            GameMode->ServerProcessNightAction(MyPlayerId, TargetId);
        }
    }
}

// 채팅 메시지 전송
void ADedicatedCharacter::SendChatMessage(const FString& Message)
{
    if (Message.IsEmpty()) return;

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    AMafiaPlayerState* MyPS = Cast<AMafiaPlayerState>(PC->PlayerState);
    if (!MyPS) return;

    ADedicatedGameMode* GameMode = Cast<ADedicatedGameMode>(GetWorld()->GetAuthGameMode());
    if (GameMode)
    {
        GameMode->ServerSendChatMessage(MyPS->GetPlayerHash(), Message);
    }
}

// 강제 이동 (멀티캐스트 RPC)
void ADedicatedCharacter::ForceMoveToLocation_Implementation(FVector Location)
{
    SetActorLocation(Location);
    TargetLocation = Location;
    bIsMovingToTarget = false;
}

// 서버에서 위치 이동 (서버 RPC)
void ADedicatedCharacter::ServerMoveToLocation_Implementation(FVector Location)
{
    SetActorLocation(Location);
}