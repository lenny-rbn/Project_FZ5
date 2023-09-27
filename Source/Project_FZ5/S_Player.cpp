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

    CanMove = true;
    CanDash = true;
    CanParry = true;
    CanAttack = true;

    IsMelee = true;
    IsDashing = false;
    IsParrying = false;
    IsAttacking = false;

    DashCD = 0.f;
    ParryCD = 0.f;
    AttackCD = 0.f;
    DashTime = 0.f;

    Player = GetCharacterMovement();
}

void AS_Player::Move(const FInputActionValue& Value)
{
    if (!IsDashing)
    {
        MoveDir = Value.Get<FVector2D>();
        MoveDir.Normalize();
        Yaw = FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f);
    }

    if (CanMove)
    {
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
    if (CanDash)
    {
        CanMove = false;
        CanDash = false;
        CanParry = false;
        IsDashing = true;
        DashCD = DashCooldown;
        DashTime = DashingTime;
        
        Player->Velocity.Z = 0.f;
        Player->BrakingDecelerationWalking = 0.f;

        if (Player->IsMovingOnGround())
            Player->SetJumpAllowed(false);

        FVector Vec = (GetActorForwardVector() * MoveDir.Y + GetActorRightVector() * MoveDir.X) * DashSpeed;
        DashVelocity = FVector(Vec.X, Vec.Y, 1.f);
    }
}

void AS_Player::Parry(const FInputActionValue& Value)
{
    if (CanParry)
    {
        CanDash = false;
        CanParry = false;
        CanAttack = false;
        IsParrying = true;
        ParryCD = ParryCooldown;
        ParryTime = ParryingTime;
    }
}

void AS_Player::ParryCancel()
{
    ParryTime = 0.f;
}

void AS_Player::Attack(const FInputActionValue& Value)
{
    if (CanAttack)
    {
        CanDash = true;
        CanParry = true;
        CanAttack = false;
        IsAttacking = true;
        AttackTime = AttackingTime;
    }
}

void AS_Player::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateStates(DeltaTime);

    FVector display = FVector(Player->Velocity.X, Player->Velocity.Y, 0.f);
    /*if (display.Length() > 600.f)
        UE_LOG(LogTemp, Warning, TEXT("%f"), display.Length());*/

    if (!CanDash)  {UE_LOG(LogTemp, Warning, TEXT("----"));}
    else           {UE_LOG(LogTemp, Warning, TEXT("Dash"));}

    if (!CanParry) {UE_LOG(LogTemp, Warning, TEXT("-----"));}
    else           {UE_LOG(LogTemp, Warning, TEXT("Parry"));}

    if (!CanAttack){UE_LOG(LogTemp, Warning, TEXT("------"));}
    else           {UE_LOG(LogTemp, Warning, TEXT("Attack"));}

    if (IsDashing)
        Player->Velocity = Player->Velocity * FVector::UpVector + DashVelocity * FVector(1, 1, 0);
}

void AS_Player::UpdateStates(float DeltaTime)
{
    if (!CanDash)
        DashCD   > 0.f ? DashCD   -= DeltaTime : ((IsParrying || IsDashing   || ParryCD > 0.f)   ? CanDash   = false : CanDash   = true);
    if (!CanParry)
        ParryCD  > 0.f ? ParryCD  -= DeltaTime : ((IsDashing  || IsParrying)                     ? CanParry  = false : CanParry  = true);
    if (!CanAttack)
        AttackCD > 0.f ? AttackCD -= DeltaTime : ((IsParrying || IsAttacking || ParryCD > 0.f)   ? CanAttack = false : CanAttack = true);

    if (DashTime > 0.f)
    {
        DashTime -= DeltaTime;
    }
    else if (IsDashing)
    {
        CanMove = true;
        CanParry = true;
        IsDashing = false;
        Player->SetJumpAllowed(true);
        Player->BrakingDecelerationWalking = Deceleration;
    }

    if (ParryTime > 0.f) { ParryTime -= DeltaTime; }
    else if (IsParrying) { IsParrying = false; ParryCD = ParryCooldown; }

    if (AttackTime > 0.f) { AttackTime -= DeltaTime; }
    else if (IsAttacking) { IsAttacking = false; AttackCD = AttackCooldown; }
}

void AS_Player::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AS_Player::Move);
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AS_Player::MoveCancel);
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AS_Player::Look);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AS_Player::Jump);
        EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Started, this, &AS_Player::Dash);
        EnhancedInputComponent->BindAction(ParryAction, ETriggerEvent::Started, this, &AS_Player::Parry);
        EnhancedInputComponent->BindAction(ParryAction, ETriggerEvent::Completed, this, &AS_Player::ParryCancel);
        EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &AS_Player::Attack);
    }
}