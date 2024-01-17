#include "S_Player.h"
#include "Engine/EngineTypes.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/InputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"


AS_Player::AS_Player()
{
    PrimaryActorTick.bCanEverTick = true;

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(RootComponent);
    SpringArm->TargetArmLength = 300.0f;
    SpringArm->bUsePawnControlRotation = true;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
    Camera->bUsePawnControlRotation = false;
}

void AS_Player::BeginPlay()
{
    Super::BeginPlay();

    if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(PlayerMappingContext, 0);
        }
    }

    IsMoving = false;

    IsDashUp = true;
    IsSlideUp = true;
    IsWallRunUp = true;

    IsGearUp = true;
    IsShootUp = true;
    IsSlashUp = true;
    IsParryUp = true;
    IsSwitchUp = true;

    item = SWORD;
    state = NEUTRAL;
    action = NONE;

    Player = GetCharacterMovement();
}

bool AS_Player::CanDash()
{
    return item == SWORD && state != WALLRUN && IsDashUp && action != PARRY && IsParryUp;
}

bool AS_Player::CanSlide()
{
    return item == GUN && Player->Velocity.Length() >= 500.f;
}

bool AS_Player::CanParry()
{
    return item == SWORD && state != DASH && IsParryUp;
}

bool AS_Player::CanShoot()
{
    return item == GUN && IsShootUp;
}

bool AS_Player::CanSlash()
{
    return item == SWORD && IsSlashUp && action != PARRY && IsParryUp;
}

bool AS_Player::CanWallRun()
{
    return IsMoving && GetWallRunDirection() != FVector::ZeroVector && !(state == DASH && Player->IsMovingOnGround());
}

void AS_Player::Move(const FInputActionValue& Value)
{
    if (state == NEUTRAL)
    {
        IsMoving = true;
        MoveDir = Value.Get<FVector2D>();
        MoveDir.Normalize();
        Yaw = FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f);

        AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), MoveDir.Y);
        AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), MoveDir.X);
    }
}

void AS_Player::MoveCancel()
{
    IsMoving = false;
    MoveDir = FVector2D(0, 1);
}

void AS_Player::Look(const FInputActionValue& Value)
{
    const FVector2D LookAxisValue = Value.Get<FVector2D>();

    if (GetController())
    {
        AddControllerYawInput(LookAxisValue.X);
        AddControllerPitchInput(LookAxisValue.Y);
    }
}

void AS_Player::Dash(const FInputActionValue& Value)
{
    if (CanDash())
    {
        state = DASH;
        IsDashUp = false;
        GetWorld()->GetTimerManager().SetTimer(DashHandler, this, &AS_Player::StopDash, DashingTime);

        Player->Velocity.Z = 0.f;

        if (Player->IsMovingOnGround())
            Player->SetJumpAllowed(false);

        FVector Vec = (GetActorForwardVector() * MoveDir.Y + GetActorRightVector() * MoveDir.X) * DashSpeed;
        DashVelocity = FVector(Vec.X, Vec.Y, 1.f);

        Player->BrakingDecelerationWalking = 0.f;
    }
    else if (CanSlide())
    {
        state = SLIDE;

        if (Player->IsMovingOnGround())
            Player->SetJumpAllowed(false);

        Player->BrakingDecelerationWalking = SlideDeceleration;
    }
}

void AS_Player::StopDash()
{
    if (state == DASH) state = NEUTRAL;
    Player->BrakingDecelerationWalking = Deceleration;
    Player->SetJumpAllowed(true);

    FTimerHandle DashCDHandler;
    FTimerDelegate DashDelegate = FTimerDelegate::CreateUObject(this, &AS_Player::AllowState, DASH);
    GetWorld()->GetTimerManager().SetTimer(DashCDHandler, DashDelegate, DashCooldown, false);
}

void AS_Player::SlideCancel()
{
    if (state == SLIDE)
    {
        state = NEUTRAL;
        Player->BrakingDecelerationWalking = Deceleration;
        Player->SetJumpAllowed(true);
    }
}

void AS_Player::Parry(const FInputActionValue& Value)
{
    if (CanParry())
    {
        action = PARRY;
        IsParryUp = false;
        GetWorld()->GetTimerManager().SetTimer(ParryHandler, this, &AS_Player::StopParry, ParryingTime);
    }
}

void AS_Player::StopParry()
{
    if (action == PARRY) action = NONE;

    FTimerHandle ParryCDHandler;
    FTimerDelegate ParryDelegate = FTimerDelegate::CreateUObject(this, &AS_Player::AllowAction, PARRY);
    GetWorld()->GetTimerManager().SetTimer(ParryCDHandler, ParryDelegate, ParryCooldown, false);
}

void AS_Player::ParryCancel()
{
    ParryHandler.Invalidate();
    StopParry();
}

void AS_Player::Attack(const FInputActionValue& Value)
{
    if (CanSlash())
    {
        action = SLASH;
        IsDashUp = true;
        IsSlashUp = false;
        GetWorld()->GetTimerManager().SetTimer(SlashHandler, this, &AS_Player::StopSlash, SlashingTime);
    }
    else if (CanShoot())
    {
        action = SLASH;
        IsShootUp = false;
        GetWorld()->GetTimerManager().SetTimer(ShootHandler, this, &AS_Player::StopShoot, ShootingTime);
    }
}

void AS_Player::StopSlash()
{
    if (action == SLASH) action = NONE;

    FTimerHandle SlashCDHandler;
    FTimerDelegate SlashDelegate = FTimerDelegate::CreateUObject(this, &AS_Player::AllowAction, SLASH);
    GetWorld()->GetTimerManager().SetTimer(SlashCDHandler, SlashDelegate, SlashCooldown, false);
}

void AS_Player::StopShoot()
{
    if (action == SHOOT) action = NONE;

    FTimerHandle ShootCDHandler;
    FTimerDelegate ShootDelegate = FTimerDelegate::CreateUObject(this, &AS_Player::AllowAction, SHOOT);
    GetWorld()->GetTimerManager().SetTimer(ShootCDHandler, ShootDelegate, ShootCooldown, false);
}

void AS_Player::TakeSword(const FInputActionValue& Value)
{
    item = SWORD;
}

void AS_Player::TakeGun1(const FInputActionValue& Value)
{
    item = GUN;
}

void AS_Player::TakeGun2(const FInputActionValue& Value)
{
    item = GUN;
}

void AS_Player::JumpButton(const FInputActionValue& Value)
{
    if (state == WALLRUN)
    {
        WallRunHandler.Invalidate();
        StopWallrun();
        FVector jumpDirection = WallHit.ImpactNormal + FVector::UpVector;
        jumpDirection.Normalize();
        Player->AddImpulse(jumpDirection * 100000.0f);
        FTimerHandle Handler;
        GetWorld()->GetTimerManager().SetTimer(Handler, this, &AS_Player::ResetAction, 1.0f);
    }
    else if (CanWallRun())
    {
        state = WALLRUN;
        GetWorld()->GetTimerManager().SetTimer(WallRunHandler, this, &AS_Player::StopWallrun, MaxWallRunTime);

        if (Player->Velocity.Size2D() < Player->MaxWalkSpeed)
            WallRunVelocity = 600.f;
        else
            WallRunVelocity = Player->Velocity.Size2D();
    }

    if (CanJump())
        Jump();
}

void AS_Player::ResetAction()
{
    action = NONE;
}

FVector AS_Player::GetWallRunDirection()
{
    FVector PlayerLocation = GetActorLocation();

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    if (GetWorld()->LineTraceSingleByChannel(WallHit, PlayerLocation, PlayerLocation - GetActorRightVector() * WallCheckDistance, ECC_Visibility, Params))
    {
        float dot = FVector::DotProduct(WallHit.ImpactNormal, GetActorForwardVector());
        if (dot < -0.1f && dot > -0.7f)
        {
            FVector WallVector = FVector::CrossProduct(WallHit.ImpactNormal, FVector::UpVector);
            WallVector.Normalize();
            return (FVector::DotProduct(WallVector, GetActorForwardVector()) > 0) ? WallVector : -WallVector;
        }
    }
    else if (GetWorld()->LineTraceSingleByChannel(WallHit, PlayerLocation, PlayerLocation + GetActorRightVector() * WallCheckDistance, ECC_Visibility, Params))
    {
        float dot = FVector::DotProduct(WallHit.ImpactNormal, GetActorForwardVector());
        if (dot < -0.1f && dot > -0.7f)
        {
            FVector WallVector = FVector::CrossProduct(WallHit.ImpactNormal, FVector::UpVector);
            WallVector.Normalize();
            return (FVector::DotProduct(WallVector, GetActorForwardVector()) > 0) ? WallVector : -WallVector;
        }
    }
    /*else if (GetWorld()->LineTraceSingleByChannel(WallHit, PlayerLocation, PlayerLocation + GetActorForwardVector() * WallCheckDistance, ECC_Visibility, Params))
    {
        float dot = FVector::DotProduct(WallHit.ImpactNormal, GetActorForwardVector());
        if (dot >= -0.7f)
        {
            FVector WallVector = FVector::CrossProduct(WallHit.ImpactNormal, FVector::UpVector);
            WallVector.Normalize();
            return (FVector::DotProduct(WallVector, GetActorForwardVector()) > 0) ? WallVector : -WallVector;
        }
    }*/
    return FVector::ZeroVector;
}

void AS_Player::StopWallrun()
{
    if (state == WALLRUN) state = NEUTRAL;
}

void AS_Player::UpdateStates(float DeltaTime)
{
    // Movement
    if (state == DASH)
    {
        Player->Velocity = Player->Velocity * FVector::UpVector + DashVelocity * FVector(1, 1, 0);
    }
    else if (state == WALLRUN)
    {
        FVector WallRunDirection = GetWallRunDirection();
        if (WallRunDirection == FVector::ZeroVector)
        {
            WallRunHandler.Invalidate();
            StopWallrun();
            return;
        }

        FVector NewVelocity = WallRunDirection * WallRunVelocity;
        NewVelocity.Z = (Player->Velocity.Z <= 0.f) ? 0.f : Player->Velocity.Z;
        Player->Velocity = NewVelocity;
    }
}

void AS_Player::AllowState(State request)
{
    switch (request)
    {
    case DASH:
        IsDashUp = true;
        break;
    case SLIDE:
        IsSlideUp = true;
        break;
    case WALLRUN:
        IsWallRunUp = true;
        break;
    default:
        break;
    }
}

void AS_Player::AllowAction(Action request)
{
    switch (request)
    {
    case SWITCH:
        IsSwitchUp = true;
        break;
    case SLASH:
        IsSlashUp = true;
        break;
    case SHOOT:
        IsShootUp = true;
        break;
    case PARRY:
        IsParryUp = true;
        break;
    case GEAR:
        IsGearUp = true;
        break;
    default:
        break;
    }
}

void AS_Player::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateStates(DeltaTime);
    
    if (!CanDash()) { UE_LOG(LogTemp, Warning, TEXT("----")); }
    else { UE_LOG(LogTemp, Warning, TEXT("Dash")); }

    if (!CanSlide()) { UE_LOG(LogTemp, Warning, TEXT("-----")); }
    else { UE_LOG(LogTemp, Warning, TEXT("Slide")); }

    if (!CanParry()) { UE_LOG(LogTemp, Warning, TEXT("-----")); }
    else { UE_LOG(LogTemp, Warning, TEXT("Parry")); }

    if (!CanShoot()) { UE_LOG(LogTemp, Warning, TEXT("-----")); }
    else { UE_LOG(LogTemp, Warning, TEXT("Shoot")); }

    if (!CanSlash()) { UE_LOG(LogTemp, Warning, TEXT("-----")); }
    else { UE_LOG(LogTemp, Warning, TEXT("Slash")); }

    if (!CanWallRun()) { UE_LOG(LogTemp, Warning, TEXT("--------")); }
    else { UE_LOG(LogTemp, Warning, TEXT("Wallrun")); }
}

void AS_Player::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInputComponent->BindAction(MoveAction,      ETriggerEvent::Triggered, this, &AS_Player::Move);
        EnhancedInputComponent->BindAction(MoveAction,      ETriggerEvent::Completed, this, &AS_Player::MoveCancel);
        EnhancedInputComponent->BindAction(LookAction,      ETriggerEvent::Triggered, this, &AS_Player::Look);
        EnhancedInputComponent->BindAction(JumpAction,      ETriggerEvent::Started,   this, &AS_Player::JumpButton);
        EnhancedInputComponent->BindAction(DashAction,      ETriggerEvent::Started,   this, &AS_Player::Dash);
        EnhancedInputComponent->BindAction(DashAction,      ETriggerEvent::Completed, this, &AS_Player::SlideCancel);
        EnhancedInputComponent->BindAction(ParryAction,     ETriggerEvent::Started,   this, &AS_Player::Parry);
        EnhancedInputComponent->BindAction(ParryAction,     ETriggerEvent::Completed, this, &AS_Player::ParryCancel);
        EnhancedInputComponent->BindAction(AttackAction,    ETriggerEvent::Started,   this, &AS_Player::Attack);
        EnhancedInputComponent->BindAction(TakeSwordAction, ETriggerEvent::Started,   this, &AS_Player::TakeSword);
        EnhancedInputComponent->BindAction(TakeGun1Action,  ETriggerEvent::Started,   this, &AS_Player::TakeGun1);
        EnhancedInputComponent->BindAction(TakeGun2Action,  ETriggerEvent::Started,   this, &AS_Player::TakeGun2);
    }
}