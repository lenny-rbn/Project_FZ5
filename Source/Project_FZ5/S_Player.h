#pragma once
#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "GameFramework/Character.h"

#include "S_Player.generated.h"

class UInputMappingContext;
class USpringArmComponent;
class UCameraComponent;
class UInputAction;

enum State { GROUNDED, JUMPING, FALLING, WALLRUN };

enum Action { NONE, DASH, SLIDE, SWITCH, PARRY, SLASH, SHOOT, SPECIAL };

enum Item { SWORD, GUN, HEAL, UTIL };

UCLASS()
class PROJECT_FZ5_API AS_Player : public ACharacter
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		USpringArmComponent* SpringArm;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		UCameraComponent* Camera;

	bool IsDashing;
	bool IsSliding;

	float DashCD;
	float ParryCD;
	float ShootCD;
	float SlashCD;
	float SwitchCD;

	float DashTime;
	float ParryTime;
	float ShootTime;
	float SlashTime;
	float SwitchTime;

	State state;
	Action action;
	Item item;

	FRotator Yaw;
	FVector2D MoveDir;
	FVector DashVelocity;

	UCharacterMovementComponent* Player;

	void UpdateStates(float DeltaTime);

	bool CanDash();
	bool CanSlide();
	bool CanParry();
	bool CanShoot();
	bool CanSlash();

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
	void TakeSword(const FInputActionValue& Value);
	void TakeGun1(const FInputActionValue& Value);
	void TakeGun2(const FInputActionValue& Value);

public:
	AS_Player();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "++Movement")
		float DashSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "++Movement")
		float Deceleration;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "++Movement")
		float SlideDeceleration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "++Duration")
		float DashingTime;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "++Duration")
		float ParryingTime;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "++Duration")
		float ShootingTime;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "++Duration")
		float SlashingTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "++Cooldown")
		float DashCooldown;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "++Cooldown")
		float ParryCooldown;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "++Cooldown")
		float ShootCooldown;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "++Cooldown")
		float SlashCooldown;

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};