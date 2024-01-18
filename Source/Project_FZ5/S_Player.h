#pragma once
#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Character.h"

#include "S_Player.generated.h"

class UInputMappingContext;
class USpringArmComponent;
class UCameraComponent;
class UInputAction;

enum Item { SWORD, GUN, HEAL, UTIL };

enum State { NEUTRAL, DASH, SLIDE, WALLRUN, WALLCLIMB, WALLJUMP };

enum Action { NONE, SWITCH, SLASH, SHOOT, PARRY, GEAR };

UCLASS()
class PROJECT_FZ5_API AS_Player : public ACharacter
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		USpringArmComponent* SpringArm;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		UCameraComponent* Camera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "++Cooldown", meta = (AllowPrivateAccess = "true"))
		float DashCooldown;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "++Cooldown", meta = (AllowPrivateAccess = "true"))
		float ParryCooldown;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "++Cooldown", meta = (AllowPrivateAccess = "true"))
		float ShootCooldown;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "++Cooldown", meta = (AllowPrivateAccess = "true"))
		float SlashCooldown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "++Duration", meta = (AllowPrivateAccess = "true"))
		float DashingTime;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "++Duration", meta = (AllowPrivateAccess = "true"))
		float ParryingTime;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "++Duration", meta = (AllowPrivateAccess = "true"))
		float ShootingTime;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "++Duration", meta = (AllowPrivateAccess = "true"))
		float SlashingTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "++Movement", meta = (AllowPrivateAccess = "true"))
		float DashSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "++Movement", meta = (AllowPrivateAccess = "true"))
		float Deceleration;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "++Movement", meta = (AllowPrivateAccess = "true"))
		float SlideDeceleration;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "++WallRide", meta = (AllowPrivateAccess = "true"))
        float WallCheckDistance;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "++WallRide", meta = (AllowPrivateAccess = "true"))
        float MaxWallRunTime;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "++WallRide", meta = (AllowPrivateAccess = "true"))
        float MaxWallClimbTime;

	bool IsMoving;

	bool IsDashUp;
	bool IsSlideUp;
	bool IsWallRunUp;

	bool IsGearUp;
	bool IsShootUp;
	bool IsSlashUp;
	bool IsParryUp;
	bool IsSwitchUp;

	float WallVelocity;

	Item item;

	State state;
	Action action;

	FRotator Yaw;
	FVector2D MoveDir;
	FVector DashVelocity;

    FHitResult WallHit;

	FTimerHandle DashHandler;
	FTimerHandle ParryHandler;
	FTimerHandle ShootHandler;
	FTimerHandle SlashHandler;
	FTimerHandle SwitchHandler;
	FTimerHandle WallRunHandler;
	FTimerHandle WallJumpHandler;
	FTimerHandle WallClimbHandler;

	UCharacterMovementComponent* Player;

	void UpdateStates(float DeltaTime);
	void AllowState(State state);
	void AllowAction(Action action);

	void StopDash();
	void StopSlash();
	void StopShoot();
	void StopParry();
	void StopWallRun();
	void StopWallJump();
	void StopWallClimb();

	bool CanDash();
	bool CanSlide();
	bool CanParry();
	bool CanShoot();
	bool CanSlash();
	bool CanWallRun();
	bool CanWallClimb();

	void ResetAction();

	FVector SetWallVector();
	FVector GetWallRunDirection();
	FVector GetWallClimbDirection();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
		UInputMappingContext* PlayerMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
		UInputAction* MoveAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
		UInputAction* LookAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
		UInputAction* JumpAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
		UInputAction* DashAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
		UInputAction* ParryAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
		UInputAction* AttackAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
		UInputAction* TakeSwordAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
		UInputAction* TakeGun1Action;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
		UInputAction* TakeGun2Action;

	void MoveCancel();
	void ParryCancel();
	void SlideCancel();
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Dash(const FInputActionValue& Value);
	void Parry(const FInputActionValue& Value);
	void Attack(const FInputActionValue& Value);
	void TakeGun1(const FInputActionValue& Value);
	void TakeGun2(const FInputActionValue& Value);
	void TakeSword(const FInputActionValue& Value);
	void JumpButton(const FInputActionValue& Value);

public:
	AS_Player();

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};