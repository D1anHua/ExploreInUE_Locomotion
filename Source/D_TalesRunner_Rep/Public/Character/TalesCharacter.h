// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/TimelineComponent.h"
#include "CoreMinimal.h"
#include "InputAction.h"
#include "GameFramework/Character.h"
// GAS
#include "AbilitySystemInterface.h"
#include "TalesCharacterMovementComponent.h"
#include "Abilities/GameplayAbility.h"
#include "System/TalesRunnerTypes.h"

#include "TalesCharacter.generated.h"



// Component
class UCameraComponent;
class USpringArmComponent;
class UTalesInventoryComponent;
class UTalesAbilitySystemCompBase;
class UTalesCharacterMovementComponent;

// GAS
class UTalesAttributeSetBase;
class UGameplayEffect;
class UGameplayAbility;

// Enhanced Input
class UInputMappingContext;
class UInputAction;
struct FInputActionInstance;

// NiagaraSystem
class UNiagaraComponent;

UCLASS()
class D_TALESRUNNER_REP_API ATalesCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ATalesCharacter(const FObjectInitializer& ObjectInitializer);
	virtual void PawnClientRestart() override;

	// Help Function
	FCollisionQueryParams GetIgnoreCharacterParams() const;
	FORCEINLINE UTalesInventoryComponent* GetTalesInventoryComponent() const { return InventoryComponent; }

	void SetSwardMesh(UStaticMesh* InSwardMesh);
	void SetShieldMesh(UStaticMesh* InShieldMesh);

	// ----------------------------------- CharacterMovementComponent ----------------------------------
	//! @note 说明: 以下一系列函数属于是 CharacterMovementComponent的接口函数, 用于设置CharacterMovementComponent的方法
	FORCEINLINE UTalesCharacterMovementComponent* GetTalesCharacterMovement() const { return TalesCharacterMovementComponent; }
	FORCEINLINE void Sprint()   const {TalesCharacterMovementComponent->SprintPressed(); }
	FORCEINLINE void UnSprint() const {TalesCharacterMovementComponent->SprintReleased(); };
	
	

#pragma region GASCourse
public:
	// Helper Function In GAS
	bool ApplyGameplayEffectToSelf(TSubclassOf<UGameplayEffect> Effect, FGameplayEffectContextHandle InEffectContext);
	FORCEINLINE FCharacterData GetCharacterData() const { return CharacterData; }
	FORCEINLINE void SetCharacterData(FCharacterData InCharacterData){ CharacterData = InCharacterData; InitFromCharacterData(CharacterData); }
protected:
	void GiveAbilities();
	void ApplyStartupEffects();

	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	UFUNCTION() void OnRep_CharacterData();

	// ~~~~~~~~~~~~~~~~~~~ IAbilitySystemInterface ~~~~~~~~~~~~~~~~~~~~~~~~~~
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	// ~~~~~~~~~~~~~~~~~ End IAbilitySystemInterface ~~~~~~~~~~~~~~~~~~~~~~~~

	UPROPERTY(ReplicatedUsing = OnRep_CharacterData)
	FCharacterData CharacterData;
	
	UPROPERTY(EditDefaultsOnly, Category = "DataAssets")
	class UCharacterDataAsset* CharacterDataAsset;

	UPROPERTY(EditDefaultsOnly)
	UTalesAbilitySystemCompBase* AbilitySystemCompBase;
	
	UPROPERTY(Transient)
	UTalesAttributeSetBase* AttributeSetBase;

	virtual void InitFromCharacterData(const FCharacterData& InCharacterData, bool bFromReplication = false);
#pragma endregion

#pragma region EnhancedInput
protected:
	/* Enhanced Input, PCInputMapping using for PC Game */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* PCInputMapping;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* ClimbInputMapping;
	
	void AddInputMappingContext(UInputMappingContext* ContextToAdd, int32 InPriority);
	void RemoveInputMappingContext(UInputMappingContext* ContextToAdd);
	// Change InputMapping
	virtual void OnPlayerEnterClimbState();
	virtual void OnPlayerExitClimbState();
	virtual void OnReachTopClimbState();

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* Input_Move;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* Input_ClimbMove;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* Input_Jump;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* Input_Sprint;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* Input_LookMouse;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* Input_ClimbHop;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* Input_Crouch;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* Input_Equip;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* Input_Drop;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* Input_Dash;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* Input_Climb;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* Input_Slide;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* Input_Prone;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Replicated)
	TObjectPtr<UTalesInventoryComponent> InventoryComponent;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<USpringArmComponent> SpringArmComp;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<UCameraComponent>    CameraComp;

	// Sprint related Comp
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	TObjectPtr<UTalesCharacterMovementComponent> TalesCharacterMovementComponent;
	
	// Timeline Comp to control FOV change, when sprint
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sprint|VFX")
	TObjectPtr<UTimelineComponent> SprintTimeLineComp;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sprint|VFX")
	TObjectPtr<UNiagaraComponent> SprintLineNiagaraComp;
	
	// Static Mesh of Sward and Shield	
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> SwardMesh;
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> ShieldMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CameraShake")
	TSubclassOf<UCameraShakeBase> SprintShake;

	
	/* Enhanced Input Function*/
	void MoveFunc(const FInputActionInstance& Instance);
	void ClimbMoveFunc(const FInputActionInstance& Instance);
	void SprintStart(const FInputActionInstance& Instance);
	void SprintStop(const FInputActionInstance& Instance);
	void LookMouse(const FInputActionInstance& Instance);
	void ClimbHop(const FInputActionInstance& Instance);
	void OnSlideStart(const FInputActionInstance& Instance);
	void OnProneStart(const FInputActionInstance& Instance);
	void OnJumpActionStart(const FInputActionInstance& Instance);
	void OnCrouchActionStart(const FInputActionInstance& Instance);
	void OnCrouchActionEnd(const FInputActionInstance& Instance);
	void OnSprintActionStart(const FInputActionInstance& Instance);
	void OnSprintActionEnd(const FInputActionInstance& Instance);
	void OnDashActionStart(const FInputActionInstance& Instance);
	void OnClimbActionStart(const FInputActionInstance& Instance);
	void OnClimbActionEnd(const FInputActionInstance& Instance);
	void OnClimbHopActionStart(const FInputActionInstance& Instance);
	
	void OnDropItemTriggered(const FInputActionInstance& Instance);
	void OnEquipItemTriggered(const FInputActionInstance& Instance);

#pragma endregion
	
public:
	// Locomotion
	virtual void Jump() override;
	virtual void Landed(const FHitResult& Hit) override;
	//! @todo 之后记得更改为private
	TEnumAsByte<EAnimEnumLandState> LandState;
	float CameraTurnRate = 0.f;
	virtual void StopJumping() override;

	//! Replicated Acceleration
	UPROPERTY(Transient, ReplicatedUsing = OnRep_ReplicatedAcceleration)
	FTalesReplicatedAcceleration ReplicatedAcceleration;
	UFUNCTION()
	void OnRep_ReplicatedAcceleration();

	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;

public:
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
// TODO: should be private
protected:

	//! @TODO Delete need
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Params")
	float DefaultFov				= 40.f;
	// Sprint Setting
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Params")
	bool  bIsSprint				= false;;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Params")
	float SprintFov				= 45.f;

	//Function to update FOV when Sprint
public:
	UPROPERTY(EditAnywhere, Category = "Sprint|Timeline")
	UCurveFloat* SprintFovChangeFloatCurve;

	bool bPressedTalesJump;
	
protected:
	// @TODO: Delete // Track used for Sprint Fov Change
	FOnTimelineFloat UpdateSprintFOVTrack;
	UFUNCTION()
	void UpdateSprintFov(float TimelineOutput);
	
private:
	// @TODO: Will Delete
	UPROPERTY()
	UCameraShakeBase* CurrentSprintShake;

	//! Interact Component
public:
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Interact")
	TSubclassOf<class UTalesInventorInteractUI> InteractUIClass;
	
	UPROPERTY()	
	class UTalesInventorInteractUI* InteractUI;

	// Interact Begin
	void ActivateInteractUI();
	void UnActivateInteractUI();

	//! Crouch Move used by GAS
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	// Gameplay Event
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Event")
	FGameplayTag JumpEventTag;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Event")
	FGameplayTagContainer InAirTags;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Event")
	FGameplayTagContainer CrouchTags;
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Event")
	FGameplayTagContainer SprintTags;
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Event")
	FGameplayTagContainer ClimbTags;
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Event")
	FGameplayTagContainer ClimbReachTags;
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Event")
	FGameplayTagContainer ClimbHopTags;
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Event")
	FGameplayTagContainer DashTags;
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Event")
	FGameplayTagContainer SlideTags;

protected:
	// Gameplay Effect
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Event")
	TSubclassOf<UGameplayEffect> CrouchStateEffect;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Event")
	TSubclassOf<UGameplayEffect> ClimbStateEffect;
};
