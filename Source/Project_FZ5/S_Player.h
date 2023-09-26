#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"

#include "S_Player.generated.h"

class UInputMappingContext;
class UInputAction;

UCLASS()
class PROJECT_FZ5_API AS_Player : public ACharacter
{
    GENERATED_BODY()

    bool CanMove;
    bool CanDash;
    bool CanParry;
    bool CanAttack;

    bool IsMelee;
    bool IsDashing;
    bool IsParrying;
    bool IsAttacking;

    float DashCD;
    float ParryCD;
    float AttackCD;

    float DashTime;
    FVector DashVelocity;

    FRotator Yaw;
    FVector2D MoveDir;

    UCharacterMovementComponent* Player;

    void SetVelocity(float Speed);
    void UpdateStates(float DeltaTime);

public:
    AS_Player();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++")
    float DashSpeed;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++")
    float DashingTime;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++")
    float DashCooldown;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++")
    float ParryCooldown;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++")
    float AttackCooldown;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++")
    float Deceleration;

    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

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
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void Dash(const FInputActionValue& Value);
    void Parry(const FInputActionValue& Value);
    void Attack(const FInputActionValue& Value);
};