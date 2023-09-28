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
enum Action { NONE, DASH, PARRY, ATTACK, SPECIAL };
enum Item { SWORD, GUN, HEAL, UTIL };

UCLASS()
class PROJECT_FZ5_API AS_Player : public ACharacter
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		USpringArmComponent* SpringArm;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		UCameraComponent* Camera;

	float DashCD;
	float ParryCD;
	float AttackCD;

	float DashTime;
	float ParryTime;
	float AttackTime;

	State state;
	Action action;
	Item item;

	FRotator Yaw;
	FVector2D MoveDir;
	FVector DashVelocity;

	UCharacterMovementComponent* Player;

	void UpdateStates(float DeltaTime);

	bool CanDash();
	bool CanParry();
	bool CanAttack();

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

	void MoveCancel();
	void ParryCancel();
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Dash(const FInputActionValue& Value);
	void Parry(const FInputActionValue& Value);
	void Attack(const FInputActionValue& Value);

public:
	AS_Player();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "++Movement")
		float DashSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "++Movement")
		float Deceleration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "++Duration")
		float DashingTime;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "++Duration")
		float ParryingTime;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "++Duration")
		float AttackingTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "++Cooldown")
		float DashCooldown;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "++Cooldown")
		float ParryCooldown;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "++Cooldown")
		float AttackCooldown;

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};