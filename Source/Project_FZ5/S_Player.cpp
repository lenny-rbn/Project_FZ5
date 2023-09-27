#include "S_Player.h"
#include "Components/InputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
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

    DashCD = 0.f;
    ParryCD = 0.f;
    AttackCD = 0.f;
    DashTime = 0.f;

    state = GROUNDED;
    action = NONE;
    item = SWORD;

    Player = GetCharacterMovement();
}

bool AS_Player::CanDash()
{
    return action == ATTACK || (action != PARRY && ParryCD < 0.f && DashCD < 0.f);
}

bool AS_Player::CanParry()
{
    return action != DASH && ParryCD < 0.f;
}

bool AS_Player::CanAttack()
{
    return action != PARRY && ParryCD < 0.f && AttackCD < 0.f;
}

void AS_Player::Move(const FInputActionValue& Value)
{
    if (action != DASH)
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
        DashCD = DashCooldown;
        DashTime = DashingTime;
        
        Player->Velocity.Z = 0.f;

        if (Player->IsMovingOnGround())
            Player->SetJumpAllowed(false);

        FVector Vec = (GetActorForwardVector() * MoveDir.Y + GetActorRightVector() * MoveDir.X) * DashSpeed;
        DashVelocity = FVector(Vec.X, Vec.Y, 1.f);
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
    if (CanAttack())
    {
        action = ATTACK;
        AttackCD = AttackCooldown;
        AttackTime = AttackingTime;

        Player->SetJumpAllowed(true);
        Player->BrakingDecelerationWalking = Deceleration;
    }
}

void AS_Player::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateStates(DeltaTime);

    if (action == DASH)
        Player->Velocity = Player->Velocity * FVector::UpVector + DashVelocity * FVector(1, 1, 0);
}

void AS_Player::UpdateStates(float DeltaTime)
{
    DashCD -= DeltaTime;
    ParryCD -= DeltaTime;
    AttackCD -= DeltaTime;

    // Dash
    if (DashTime > 0.f)
    {
        DashTime -= DeltaTime;
        Player->BrakingDecelerationWalking = 0.f;
    }
    else if (action == DASH)
    {
        action = NONE;
        Player->SetJumpAllowed(true);
        Player->BrakingDecelerationWalking = Deceleration;
    }

    // Parry
    if (ParryTime > 0.f) ParryTime -= DeltaTime;
    else if (action == PARRY) action = NONE;

    // Attack
    if (AttackTime > 0.f) AttackTime -= DeltaTime;
    else if (action == ATTACK) action = NONE;
}

void AS_Player::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInputComponent->BindAction(MoveAction,   ETriggerEvent::Triggered, this, &AS_Player::Move);
        EnhancedInputComponent->BindAction(MoveAction,   ETriggerEvent::Completed, this, &AS_Player::MoveCancel);
        EnhancedInputComponent->BindAction(LookAction,   ETriggerEvent::Triggered, this, &AS_Player::Look);
        EnhancedInputComponent->BindAction(JumpAction,   ETriggerEvent::Started,   this, &AS_Player::Jump);
        EnhancedInputComponent->BindAction(DashAction,   ETriggerEvent::Started,   this, &AS_Player::Dash);
        EnhancedInputComponent->BindAction(ParryAction,  ETriggerEvent::Started,   this, &AS_Player::Parry);
        EnhancedInputComponent->BindAction(ParryAction,  ETriggerEvent::Completed, this, &AS_Player::ParryCancel);
        EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started,   this, &AS_Player::Attack);
    }
}