﻿
#pragma once
#include "GameFramework/CharacterMovementComponent.h"
#include "TalesCharacterMovementComponent.generated.h"


class ATalesCharacter;
/*!
 * @name ECustomMovementMode
 * My Custom Movement Mode
 * In EngineTypes.h
 * @brief Slide Move mode
 */
UENUM(BlueprintType)
enum ECustomMovementMode
{
	CMOVE_None			UMETA(Hidden),
	CMOVE_Slide			UMETA(DisplayName = "Slide"),
	CMOVE_Prone			UMETA(DisplayName = "Prone"),
	CMOVE_Max			UMETA(Hidden),
};

/*!
 * @name UTalesCharacterMovementComponent
 * Try to process the Acceleration Replicated
 */
UCLASS(Config = Game)
class D_TALESRUNNER_REP_API UTalesCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

private:
	//! Sprint_MaxWalkSpeed
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Speed")
	float MaxSprintSpeed = 750.f;

	//! Slide Parameter
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Slide")
	float MinSlideSpeed = 100;
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Slide")
	float MaxSlideSpeed = 400.f;
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Slide")
	float SlideEnterImpulse = 400.f;
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Slide")
	float SlideGravityForce = 4000;
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Slide")
	float SlideFriction = .06f;
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Slide")
	float BrakingDecelerationSliding = 1000.f;

	//! Slide Parameter
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Slide")
	float ProneEnterHoldDuration = .2f;
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Slide")
	float ProneSlideEnterImpulse = 300.f;
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Slide")
	float ProneMaxSpeed = 300.f;
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Slide")
	float BrakingDecelerationProne = 2500.f;
	
	
	UPROPERTY(Transient)
	ATalesCharacter* TalesCharacterOwner;
	
	//! Sprint begin Logical ---- network safely
	bool Safe_bWantToSprint;
	bool Safe_bPrevWantsToCrouch;
	bool Safe_bWantsToProne;

	FTimerHandle TimerHandle_EnterProne;
public:
	UTalesCharacterMovementComponent();
	
	virtual void InitializeComponent() override;

	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	virtual bool IsMovingOnGround() const override;
	virtual bool CanCrouchInCurrentState() const override;
	virtual float GetMaxSpeed() const override;
	virtual float GetMaxBrakingDeceleration() const override;
	
protected:
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;


public:
	UFUNCTION(BlueprintCallable)
	void SprintPressed();
	UFUNCTION(BlueprintCallable)
	void SprintReleased();
	UFUNCTION(BlueprintCallable)
	void CrouchPressed();
	UFUNCTION(BlueprintCallable)
	void CrouchReleased();
	UFUNCTION(BlueprintPure)
	bool IsCustomMovementMode(ECustomMovementMode InCustomMocementMode) const;
	UFUNCTION(BlueprintPure)
	bool IsMovementMode(EMovementMode InMovementMode) const;

private:
	//! SaveMove snapshot
	class FSavedmove_Tales : public FSavedMove_Character
	{
		typedef  FSavedMove_Character Super;
	public:
		enum CompressedFlag
		{
			FLAG_Sprint    = 0x10,
			FLAG_Custom_1  = 0x20,
			FLAG_Custom_2  = 0x40,
			FLAG_Custom_3  = 0x80,
		};
		//! 用来说明这个值只占一位
		uint8 Saved_bWantsToSprint:1;

		// Other Variables
		uint8 Saved_bPrevWantsToCrouch:1;
		uint8 Saved_bWantsToProne:1;

		virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
		virtual void Clear() override;
		virtual uint8 GetCompressedFlags() const override;
		virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override;
		virtual void PrepMoveFor(ACharacter* C) override;
	};

	class FNetworkPredictionData_Client_Tales : public FNetworkPredictionData_Client_Character
	{
	public:
		FNetworkPredictionData_Client_Tales(const UCharacterMovementComponent& ClientMovement);
		typedef FNetworkPredictionData_Client_Character Super;
		virtual FSavedMovePtr AllocateNewMove() override;
	};

	/*
	 * @brief Actor's Custom Movement Mode(Slide, etc)
	 * Slide: can Slide when we on the slope
	*/


	//! Slide Helper Function
	// void EnterSlide();
	void EnterSlide(EMovementMode PrevMode, ECustomMovementMode PrevCustomMode);
	void ExitSlide();
	bool CanSlide()const;
	void PhysSlide(float deltaTime, int32 Iterations);
	
	//! Prone Helper Function
	void TryEnterProne(){Safe_bWantsToProne = true;};
	UFUNCTION(Server, Reliable) void Server_EnterProne();
	
	void EnterProne(EMovementMode PrevMode, ECustomMovementMode PrevCustomMode);
	void ExitProne();
	bool CanProne() const;
	void PhysProne(float deltaTime, int32 Iterations);
};



