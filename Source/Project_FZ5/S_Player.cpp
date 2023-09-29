#include "S_Player.h"
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

    IsDashing = false;

    DashCD = 0.f;
    ParryCD = 0.f;
    ShootCD = 0.f;
    SlashCD = 0.f;
    SwitchCD = 0.f;

    DashTime = 0.f;
    ParryTime = 0.f;
    ShootTime = 0.f;
    SlashTime = 0.f;
    SwitchTime = 0.f;

    state = GROUNDED;
    action = NONE;
    item = SWORD;

    IsWallRunning = false;

    Player = GetCharacterMovement();
}

bool AS_Player::CanDash()
{
    return item == SWORD && (action == SLASH || (action != PARRY && ParryCD < 0.f && DashCD < 0.f));
}

bool AS_Player::CanSlide()
{
    return item == GUN;
}

bool AS_Player::CanParry()
{
    return item == SWORD && action != DASH && ParryCD < 0.f;
}

bool AS_Player::CanShoot()
{
    return item == GUN && ShootCD < 0.f && SwitchCD < 0.f;
}

bool AS_Player::CanSlash()
{
    return item == SWORD && action != PARRY && ParryCD < 0.f && SlashCD < 0.f;
}

void AS_Player::Move(const FInputActionValue& Value)
{
    if (action != DASH && !IsSliding && !IsWallRunning)
    {
        MoveDir = Value.Get<FVector2D>();
        MoveDir.Normalize();
        Yaw = FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f);

        AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), MoveDir.Y);
        AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), MoveDir.X);
    }
}

void AS_Player::MoveCancel()
{
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
        action = DASH;
        IsDashing = true;
        DashCD = DashCooldown;
        DashTime = DashingTime;

        Player->Velocity.Z = 0.f;

        if (Player->IsMovingOnGround())
            Player->SetJumpAllowed(false);

        FVector Vec = (GetActorForwardVector() * MoveDir.Y + GetActorRightVector() * MoveDir.X) * DashSpeed;
        DashVelocity = FVector(Vec.X, Vec.Y, 1.f);
    }
    else if (CanSlide())
    {
        action = SLIDE;
        IsSliding = true;
    }
}

void AS_Player::SlideCancel()
{
    if (IsSliding)
    {
        if (action == SLIDE) action = NONE;
        IsSliding = false;
        Player->BrakingDecelerationWalking = Deceleration;
    }
}

void AS_Player::Parry(const FInputActionValue& Value)
{
    if (CanParry())
    {
        action = PARRY;
        ParryCD = ParryCooldown;
        ParryTime = ParryingTime;
    }
}

void AS_Player::ParryCancel()
{
    ParryCD -= ParryTime;
    ParryTime = 0.f;
}

void AS_Player::Attack(const FInputActionValue& Value)
{
    if (CanSlash())
    {
        action = SLASH;
        SlashCD = SlashCooldown;
        SlashTime = SlashingTime;
    }
    else if (CanShoot())
    {
        action = SLASH;
        ShootCD = ShootCooldown;
        ShootTime = ShootingTime;
    }
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
    if (IsWallRunning)
    {
        IsWallRunning = false;
    }
    else if (CanWallRun())
    {
        IsWallRunning = true;
        WallRunVelocity = Player->Velocity.Size2D();
    }
    Jump();
}

bool AS_Player::CanWallRun()
{
    return action == NONE && GetWallRunDirection() != FVector::ZeroVector;
}

FVector AS_Player::GetWallRunDirection()
{
    FVector PlayerLocation = GetActorLocation();

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    if (GetWorld()->LineTraceSingleByChannel(LeftWallHit, PlayerLocation, PlayerLocation - GetActorRightVector() * WallCheckDistance, ECC_Visibility, Params))
    {
        float dot = FVector::DotProduct(LeftWallHit.ImpactNormal, GetActorForwardVector());
        if (dot < -0.1f && dot > -0.9f)
        {
            FVector WallVector = FVector::CrossProduct(LeftWallHit.ImpactNormal, FVector::UpVector);
            WallVector.Normalize();
            return (FVector::DotProduct(WallVector, GetActorForwardVector()) > 0) ? WallVector : -WallVector;
        }
    }
    else if (GetWorld()->LineTraceSingleByChannel(RightWallHit, PlayerLocation, PlayerLocation + GetActorRightVector() * WallCheckDistance, ECC_Visibility, Params))
    {
        float dot = FVector::DotProduct(RightWallHit.ImpactNormal, GetActorForwardVector());
        if (dot < -0.1f && dot > -0.9f)
        {
            FVector WallVector = FVector::CrossProduct(RightWallHit.ImpactNormal, FVector::UpVector);
            WallVector.Normalize();
            return (FVector::DotProduct(WallVector, GetActorForwardVector()) > 0) ? WallVector : -WallVector;
        }
    }
    return FVector::ZeroVector;
}

void AS_Player::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateStates(DeltaTime);

    if (!CanDash()) { UE_LOG(LogTemp, Warning, TEXT("----")); }
    else { UE_LOG(LogTemp, Warning, TEXT("Dash")); }

    if (!IsSliding) { UE_LOG(LogTemp, Warning, TEXT("----")); }
    else { UE_LOG(LogTemp, Warning, TEXT("Slide")); }

    if (!CanParry()) { UE_LOG(LogTemp, Warning, TEXT("-----")); }
    else { UE_LOG(LogTemp, Warning, TEXT("Parry")); }

    if (!CanShoot()) { UE_LOG(LogTemp, Warning, TEXT("------")); }
    else { UE_LOG(LogTemp, Warning, TEXT("Shoot")); }

    if (!CanSlash()) { UE_LOG(LogTemp, Warning, TEXT("------")); }
    else { UE_LOG(LogTemp, Warning, TEXT("Slash")); }

    if (!IsWallRunning) { UE_LOG(LogTemp, Warning, TEXT("------")); }
    else { UE_LOG(LogTemp, Warning, TEXT("Wallrun")); }

    if (IsDashing)
    {
        Player->Velocity = Player->Velocity * FVector::UpVector + DashVelocity * FVector(1, 1, 0);
    }
    else if (IsWallRunning)
    {
        FVector WallRunDirection = GetWallRunDirection();
        if (WallRunDirection == FVector::ZeroVector)
        {
            IsWallRunning = false;
            return;
        }

        FVector NewVelocity = WallRunDirection * WallRunVelocity;
        NewVelocity.Z = (Player->Velocity.Z < 0.f) ? 0.f : Player->Velocity.Z;
        Player->Velocity = NewVelocity;
    }
}

void AS_Player::UpdateStates(float DeltaTime)
{
    DashCD -= DeltaTime;
    ParryCD -= DeltaTime;
    ShootCD -= DeltaTime;
    SlashCD -= DeltaTime;
    SwitchCD -= DeltaTime;

    // Dash
    if (DashTime > 0.f)
    {
        DashTime -= DeltaTime;
        Player->BrakingDecelerationWalking = 0.f;
    }
    else if (IsDashing)
    {
        if (action == DASH) action = NONE;
        IsDashing = false;
        Player->SetJumpAllowed(true);
        Player->BrakingDecelerationWalking = Deceleration;
    }

    // Slide
    if (IsSliding) Player->BrakingDecelerationWalking = SlideDeceleration;

    // Parry
    if (ParryTime > 0.f) ParryTime -= DeltaTime;
    else if (action == PARRY) action = NONE;

    // Attack
    if (SlashTime > 0.f) SlashTime -= DeltaTime;
    else if (action == SLASH) action = NONE;
    if (ShootTime > 0.f) ShootTime -= DeltaTime;
    else if (action == SHOOT) action = NONE;

    // Switch
    if (SwitchTime > 0.f) SwitchTime -= DeltaTime;
    else if (action == SWITCH) action = NONE;
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