#include "S_Player.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "KismetProceduralMeshLibrary.h"

#include "Engine/EngineTypes.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/InputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"


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

    ////////////////////////////////////////////////////////////////

    //root = CreateDefaultSubobject<USceneComponent>(TEXT("EmptyRoot"));
    //RootComponent = root;

    // Create the slicing plane sub object.
    SlicingPlane = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SlicingMesh"));
    SlicingPlane->SetupAttachment(RootComponent/*FirstPersonCameraComponent*/);
    SlicingPlane->SetVisibility(false);
    // const float slicingDistY = 1000;
    // const float slicingDistX = (FVector::ForwardVector * slicingDistY).RotateAngleAxis(FirstPersonCameraComponent->FieldOfView * 0.5f, FVector::UpVector).X;
    // SlicingPlane->SetWorldScale3D({ slicingDistX, slicingDistY, 1 });
    SlicingPlane->SetWorldScale3D({ 3, 2, 1 });
    SlicingPlane->SetRelativeRotation(FRotator(0, 0, 90));

    ////////////////////////////////////////////////////////////////
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

    WallReset = false;

    item = SWORD;
    state = NEUTRAL;
    action = NONE;

    Player = GetCharacterMovement();

    initialRotation = SlicingPlane->GetRelativeRotation();
}

#pragma region ENUM...
void AS_Player::ResetAction()
{
    action = NONE;
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

void AS_Player::AddStateCooldown(State State, float Cooldown)
{
    FTimerHandle NewHandler;
    FTimerDelegate NewDelegate = FTimerDelegate::CreateUObject(this, &AS_Player::AllowState, State);
    GetWorld()->GetTimerManager().SetTimer(NewHandler, NewDelegate, Cooldown, false);
}

void AS_Player::AddActionCooldown(Action Action, float Cooldown)
{
    FTimerHandle NewHandler;
    FTimerDelegate NewDelegate = FTimerDelegate::CreateUObject(this, &AS_Player::AllowAction, Action);
    GetWorld()->GetTimerManager().SetTimer(NewHandler, NewDelegate, Cooldown, false);
}
#pragma endregion

#pragma region INVENTORY...
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
#pragma endregion

#pragma region CONDITIONS...
bool AS_Player::CanDash()
{
    return item == SWORD && (state == NEUTRAL || !IsSlashUp) && IsDashUp && IsParryUp;
}

bool AS_Player::CanSlide()
{
    return item == GUN && Player->Velocity.Length() >= 500.f;
}

bool AS_Player::CanParry()
{
    return item == SWORD && state != DASH && IsParryUp && state != WALLJUMP;
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

bool AS_Player::CanWallJump()
{
    return state == WALLRUN || state == WALLCLIMB || (WallReset && GetWallClimbDirection() != FVector::ZeroVector);
}

bool AS_Player::CanWallClimb()
{
    return IsMoving && GetWallClimbDirection() != FVector::ZeroVector && Player->IsMovingOnGround();
}
#pragma endregion

#pragma region MOVE...
void AS_Player::MoveStart()
{
    IsMoving = true;
}

void AS_Player::Move(const FInputActionValue& Value)
{
    MoveDir = Value.Get<FVector2D>();
    MoveDir.Normalize();

    if (state == NEUTRAL)
    {
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
#pragma endregion

#pragma region DASH INPUT...
void AS_Player::Dash(const FInputActionValue& Value)
{
    if (CanDash())
    {
        AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this] { OnAttack(); });

        if (state == WALLJUMP) WallReset = true;

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
        Player->BrakingDecelerationWalking = SlideDeceleration;
    }
}

void AS_Player::StopDash()
{
    if (state == DASH) state = NEUTRAL;
    Player->BrakingDecelerationWalking = Deceleration;
    Player->SetJumpAllowed(true);

    AddStateCooldown(DASH, DashCooldown);
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
#pragma endregion

#pragma region PARRY INPUT...
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
    AddActionCooldown(PARRY, ParryCooldown);
}

void AS_Player::ParryCancel()
{
    ParryHandler.Invalidate();
    StopParry();
}
#pragma endregion

#pragma region ATTACK INPUT...
void AS_Player::Attack(const FInputActionValue& Value)
{
    if (CanSlash())
    {
        AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this] { OnAttack(); });
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

        TraceCameraToTarget();
    }

    StopWallRun();
    StopWallClimb();
}

void AS_Player::TraceCameraToTarget()
{
    FHitResult Hit;
    FVector Start = Camera->GetComponentLocation();
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + Camera->GetForwardVector() * ShootCheckDistance, ECC_Visibility, Params) && Hit.Distance > SpringArm->TargetArmLength)
    {
        if (Hit.GetComponent()->ComponentHasTag("Destructible"))
            DrawDebugLine(GetWorld(), SpringArm->GetComponentLocation() - (FVector::ZAxisVector * 50.0f), Hit.Location, FColor(0, 0, 255), false, 1, 0, 10);
        else if (Hit.GetComponent()->ComponentHasTag("Player"))
            DrawDebugLine(GetWorld(), SpringArm->GetComponentLocation() - (FVector::ZAxisVector * 50.0f), Hit.Location, FColor(255, 0, 0), false, 1, 0, 10);
        else
            DrawDebugLine(GetWorld(), SpringArm->GetComponentLocation() - (FVector::ZAxisVector * 50.0f), Hit.Location, FColor(0, 255, 0), false, 1, 0, 10);
    }
}

void AS_Player::StopSlash()
{
    if (action == SLASH) action = NONE;
    AddActionCooldown(SLASH, SlashCooldown);
}

void AS_Player::StopShoot()
{
    if (action == SHOOT) action = NONE;
    AddActionCooldown(SHOOT, ShootCooldown);
}
#pragma endregion

#pragma region JUMP INPUT...
void AS_Player::JumpButton(const FInputActionValue& Value)
{
    if (CanWallJump())
    {
        StopWallRun();
        StopWallClimb();
        state = WALLJUMP;
        if (WallReset) Player->Velocity = FVector::UpVector * 600.f;
        else Player->Velocity.Z = 0.f;
        IsDashUp = false;
        WallReset = false;
        LastWallHit = WallHit;
        FVector JumpDir = WallHit.ImpactNormal + FVector::UpVector;
        JumpDir.Normalize();
        Player->AddImpulse(JumpDir * WallForce);
        GetWorld()->GetTimerManager().SetTimer(WallJumpHandler, this, &AS_Player::StopWallJump, WallJumpTime);
    }
    else if (CanWallRun())
    {
        state = WALLRUN;
        IsDashUp = true;
        WallReset = false;
        GetWorld()->GetTimerManager().SetTimer(WallRunHandler, this, &AS_Player::StopWallRun, MaxWallRunTime);

        if (Player->Velocity.Size2D() < Player->MaxWalkSpeed)
            WallVelocity = 600.f;
        else
            WallVelocity = Player->Velocity.Size2D();
    }
    else if (CanWallClimb())
    {
        state = WALLCLIMB;
        WallReset = false;
        GetWorld()->GetTimerManager().SetTimer(WallClimbHandler, this, &AS_Player::StopWallClimb, MaxWallClimbTime);

        WallVelocity = 600.f;
    }

    if (CanJump())
        Jump();
}

void AS_Player::StopWallJump()
{
    if (state == WALLJUMP) 
    {
        state = NEUTRAL;
        IsDashUp = true;
    }
}

void AS_Player::StopWallRun()
{
    if (state == WALLRUN) state = NEUTRAL;
}

void AS_Player::StopWallClimb()
{
    if (state == WALLCLIMB) state = NEUTRAL;
}
#pragma endregion

#pragma region WALLRIDE...
FVector AS_Player::SetWallVector()
{
    FVector WallVector = FVector::CrossProduct(WallHit.ImpactNormal, FVector::UpVector);
    WallVector.Normalize();
    return (FVector::DotProduct(WallVector, GetActorForwardVector()) > 0) ? WallVector : -WallVector;
}

FVector AS_Player::GetWallRunDirection()
{
    FVector PlayerLocation = GetActorLocation();
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    if (GetWorld()->LineTraceSingleByChannel(WallHit, PlayerLocation, PlayerLocation - GetActorRightVector() * WallCheckDistance, ECC_Visibility, Params))
    {
        if (WallHit.GetActor() == LastWallHit.GetActor() && WallHit.ImpactNormal == LastWallHit.ImpactNormal)
            return FVector::ZeroVector;

        float dot = FVector::DotProduct(WallHit.ImpactNormal, GetActorForwardVector());
        if (dot < -0.1f && dot > -0.7f)
            return SetWallVector();
    }
    else if (GetWorld()->LineTraceSingleByChannel(WallHit, PlayerLocation, PlayerLocation + GetActorRightVector() * WallCheckDistance, ECC_Visibility, Params))
    {
        if (WallHit.GetActor() == LastWallHit.GetActor() && WallHit.ImpactNormal == LastWallHit.ImpactNormal)
            return FVector::ZeroVector;

        float dot = FVector::DotProduct(WallHit.ImpactNormal, GetActorForwardVector());
        if (dot < -0.1f && dot > -0.7f)
            return SetWallVector();
    }

    return FVector::ZeroVector;
}

FVector AS_Player::GetWallClimbDirection()
{
    FVector PlayerLocation = GetActorLocation();

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    if (GetWorld()->LineTraceSingleByChannel(WallHit, PlayerLocation, PlayerLocation + GetActorForwardVector() * WallCheckDistance, ECC_Visibility, Params))
        if (FVector::DotProduct(WallHit.ImpactNormal, GetActorForwardVector()) <= -0.7f)
            return FVector::UpVector;

    return FVector::ZeroVector;
}
#pragma endregion

void AS_Player::UpdateStates(float DeltaTime)
{
    if (state == DASH)
    {
        Player->Velocity = Player->Velocity * FVector::UpVector + DashVelocity * FVector(1, 1, 0);
    }
    else if (state == WALLRUN)
    {
        FVector Dir = GetWallRunDirection();
        if (Dir == FVector::ZeroVector)
        {
            LastWallHit = WallHit;
            WallRunHandler.Invalidate();
            StopWallRun();
            return;
        }

        FVector NewVelocity = Dir * WallVelocity;
        NewVelocity.Z = (Player->Velocity.Z <= 0.f) ? 0.f : Player->Velocity.Z;
        Player->Velocity = NewVelocity;

        Player->AddForce(-WallHit.ImpactNormal * WallForce);
    }
    else if (state == WALLCLIMB)
    {
        FVector Dir = GetWallClimbDirection();
        if (Dir == FVector::ZeroVector)
        {
            WallRunHandler.Invalidate();
            StopWallClimb();
            return;
        }

        Player->Velocity = Dir * WallVelocity;
    }
}

void AS_Player::Landed(const FHitResult& Hit)
{
    Super::Landed(Hit);
    LastWallHit = Hit;
    WallReset = false;
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
}

void AS_Player::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInputComponent->BindAction(MoveAction,      ETriggerEvent::Started,   this, &AS_Player::MoveStart);
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

void AS_Player::OnAttack()
{
    TimerHandles.Empty();
    TArray<UPrimitiveComponent*> OverlappedComponents = {};
    UKismetSystemLibrary::ComponentOverlapComponents(SlicingPlane, SlicingPlane->GetComponentTransform(),
        { UEngineTypes::ConvertToObjectType(ECC_WorldStatic) }, nullptr, {}, OverlappedComponents);

    for (UPrimitiveComponent* Component : OverlappedComponents)
    {
        UProceduralMeshComponent* ProceduralMesh = Cast<UProceduralMeshComponent>(Component);
        if (!ProceduralMesh || !ProceduralMesh->IsValidLowLevel()) continue;
        AS_SlicedMesh* SliceableMesh = Cast<AS_SlicedMesh>(Component->GetOwner());
        if (!SliceableMesh || !SliceableMesh->IsValidLowLevel()) continue;

        const FVector CamLocation = SlicingPlane->GetComponentLocation();// FirstPersonCameraComponent->GetComponentLocation();
        const FVector CamUpVector = SlicingPlane->GetUpVector();// FirstPersonCameraComponent->GetUpVector();
        const FVector ProcMeshLoc = ProceduralMesh->GetComponentLocation();
        const FVector ProcMeshLocPlane = UKismetMathLibrary::ProjectPointOnToPlane(ProcMeshLoc, CamLocation, CamUpVector);

        FHitResult HitResult;
        TArray<AActor*> CollisionQuery;
        CollisionQuery.Add(this);
        UKismetSystemLibrary::LineTraceSingle(GetWorld(), CamLocation, ProcMeshLocPlane, TraceTypeQuery1, false, CollisionQuery, EDrawDebugTrace::Type::None, HitResult, true);
        if (!HitResult.IsValidBlockingHit()) continue;

        //FTimerHandle Handle;
        //GetWorldTimerManager().SetTimer(Handle, [=]() { SliceableMesh->Slice(ProceduralMesh, HitResult.ImpactPoint, CamUpVector); }, 0.6f, false);
        SliceableMesh->Slice(ProceduralMesh, HitResult.ImpactPoint, CamUpVector);

        //TimerHandles.Add(Handle);
    }
}