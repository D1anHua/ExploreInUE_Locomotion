﻿
#pragma once
#include "GameFramework/CharacterMovementComponent.h"
#include "TalesCharacterMovementComponent.generated.h"

DECLARE_DELEGATE(FOnEnterClimbState);
DECLARE_DELEGATE(FOnExitClimbState);
DECLARE_DELEGATE(FOnArriveTopState);

class ATalesCharacter;
class UTalesCharacterAnimInstance;
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
	CMOVE_Climb         UMETA(DisplayName = "Climb"),
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

public:
	FORCEINLINE float GetVelocityZoom() const {return VelocityZoom; }
	FORCEINLINE void SetVelocityZoom(float InVelocityZoom) { VelocityZoom = InVelocityZoom; }
	//! Only On Simulator
	FORCEINLINE void SetReplicatedAcceleration(const FVector& InAcceleration)
	{
		bHasReplicatedAcceleration = true;
		Acceleration = InAcceleration;
	}
	
private:
	//! Sprint_MaxWalkSpeed
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Speed")
	float MaxSprintSpeed = 750.f;
	// Just Save Crouch half height
	float CapsuleHalfHeightCrouch;

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

	//! Prone Parameter
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Slide")
	float ProneEnterHoldDuration = .4f;
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Slide")
	float ProneSlideEnterImpulse = 300.f;
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Slide")
	float ProneMaxSpeed = 300.f;
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Slide")
	float BrakingDecelerationProne = 2500.f;

	//! Mantle
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Mantle")
	float MantleMaxDistance = 230;
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Mantle")
	float MantleReachHeight = 50;
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Mantle")
	float MantleMinDepth = 30;
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Mantle")
	float MantleMinWallSteepnessAngle = 75;
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Mantle")
	float MantleMaxSurfaceAngle = 40;
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Mantle")
	float MantleMaxAlignmentAngle = 45;
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Montage")
	UAnimMontage* TallMantleMontage;
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Montage")
	UAnimMontage* TransitionTallMantleMontage;
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Montage")
	UAnimMontage* ShortMantleMontage;
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Montage")
	UAnimMontage* TransitionShortMantleMontage;
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Montage")
	UAnimMontage* ProxyTallMantleMontage;
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Montage")
	UAnimMontage* ProxyShortMantleMontage;
	// @TODO Really need ?	
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Mantle")
	uint8 MantleScanNumber = 6;

	// Climb	
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Climb")
	float MaxClimbSpeed = 150.f;
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Climb")
	float BrakingDecelerationClimbing = 1000.f;
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Climb")
	float ClimbReachDistance = 200.f;
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Climb")
	float ClimbDownWalkableSurfaceTraceOffset = 30.f;
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Climb")
	float ClimbDownLedgeTraceOffset = 80.f;
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Delete")
	UAnimMontage* TransitionClimbUpMontage;
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Delete")
	UAnimMontage* TransitionClimbDownMontage;
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Climb")
	UAnimMontage* OutClimbMontage;
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Climb")
	UAnimMontage* ProxyClimbStartMontage;
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Climb")
	TArray<UAnimMontage*> ClimbHopMontage;
	UPROPERTY(Transient)
	ATalesCharacter* TalesCharacterOwner;
	
	//! Sprint begin Logical ---- network safely
	UPROPERTY(ReplicatedUsing = OnRep_Sprint)
	uint8 Safe_WantToSprint  : 1;
	uint8 Safe_WantsToProne  : 1;
	uint8 Safe_WantsToClimb  : 1;
	uint8 Safe_WantsToSlide  : 1;
	uint8 Safe_PrevWantsToCrouch : 1;
	uint8 Safe_HadAnimRootMotion : 1;
	uint8 Safe_TransitionFinished : 1;
	uint8 Safe_ClimbTransitionFinished : 1;

	//! MovementComponent
	FVector LastUpdateAcceleration = FVector(0.f, 0.f, 0.f);

	bool bHasReplicatedAcceleration = false;

	FTimerHandle TimerHandle_EnterProne;
	TSharedPtr<FRootMotionSource_MoveToForce> TransitionRMS;
	UPROPERTY(Transient)
	UAnimMontage* TransitionQueuedMontage;
	float TransitionQueuedMontageSpeed;
	int TransitionRMS_ID;
	FString MontageName;

protected:
	//! @brief 用来设置速度的倍率
	float VelocityZoom = 1.f;
	
public:
	// Replicated
	UPROPERTY(ReplicatedUsing = OnRep_ShortMantle) bool Proxy_bShortMantle;
	UPROPERTY(ReplicatedUsing = OnRep_TallMantle) bool Proxy_bTallMantle;
	UPROPERTY(ReplicatedUsing = OnRep_ClimbStart) bool Proxy_bClimbStart;
	// Delegates
	FOnEnterClimbState OnEnterClimbStateDelegate;
	FOnExitClimbState  OnExitClimbStateDelegate;
	FOnArriveTopState  OnArriveTopStateDelegate;
	
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
	UFUNCTION() void OnRep_ShortMantle();
	UFUNCTION() void OnRep_TallMantle();
	UFUNCTION() void OnRep_ClimbStart();
	UFUNCTION() void OnRep_Sprint();
	

public:
	// Initial Function
	UTalesCharacterMovementComponent();
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	
	UPROPERTY()
	UAnimInstance* OwningPlayerAnimInstance;
	
	// Helper Function
	UFUNCTION(BlueprintPure) bool IsClimbing() const { return IsCustomMovementMode(CMOVE_Climb); }
	UFUNCTION(BlueprintPure) bool IsSlide() const { return IsCustomMovementMode(CMOVE_Slide); }
	UFUNCTION(BlueprintPure) bool IsProne() const { return IsCustomMovementMode(CMOVE_Prone); }
	UFUNCTION(BlueprintPure) bool IsSprint() const { return  IsMovementMode(MOVE_Walking) && Safe_WantToSprint && !IsCrouching(); }
	UFUNCTION(BlueprintPure) FVector GetLastUpdateAcceleration() const { return  LastUpdateAcceleration; }

	UFUNCTION(BlueprintPure)
	bool IsCustomMovementMode(ECustomMovementMode InCustomMocementMode) const;
	UFUNCTION(BlueprintPure)
	bool IsMovementMode(EMovementMode InMovementMode) const;
	UFUNCTION()
	FORCEINLINE FVector GetClimbableSurfaceNormal() const { return CurrentClimbablefaceNormal; }

	UFUNCTION(BlueprintPure)
	FVector GetUnRotatedClimbVelocity() const;

	virtual bool IsMovingOnGround() const override;
	virtual bool CanCrouchInCurrentState() const override;
	virtual float GetMaxSpeed() const override;
	virtual float GetMaxBrakingDeceleration() const override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	// 原本的SimulateMovement会更新Acceleration, 但是这个Acceleration是不对的
	virtual void SimulateMovement(float DeltaTime) override;
	virtual void PerformMovement(float DeltaTime) override;
	
protected:
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	virtual void UpdateCharacterStateAfterMovement(float DeltaSeconds) override;
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	
public:
	bool CanSlide()const;
private:
	// --------------------------------------- Slide Mode -------------------------------------------------
	//! Slide Helper Function
	void EnterSlide(EMovementMode PrevMode, ECustomMovementMode PrevCustomMode);
	void ExitSlide();
	void PhysSlide(float deltaTime, int32 Iterations);

	// --------------------------------------- Prone Mode -------------------------------------------------
	//! Prone Helper Function
	void OnTryEnterProne(){Safe_WantsToProne = true;};
	//! @TODO Delete
	UFUNCTION(Server, Reliable) void Server_EnterProne();
	
	void EnterProne(EMovementMode PrevMode, ECustomMovementMode PrevCustomMode);
	void ExitProne();
	bool CanProne() const;
	void PhysProne(float deltaTime, int32 Iterations);
	
	// --------------------------------------    Mantle    ------------------------------------------------
	bool TryMantle();
	FVector GetMantleStartLocation(FHitResult FrontHit, FHitResult SurfaceHit, bool bTallMantle) const;

	// --------------------------------------    Climb    ------------------------------------------------
	FVector CurrentClimbableSurfaceLocation;
	FVector CurrentClimbablefaceNormal;
	void EnterClimb(EMovementMode PrevMode, ECustomMovementMode PrevCustomMode);
	void ExitClimb();
	void PhysClimb(float deltaTime, int32 Iterations);

	// @TODO: 弃用
	UFUNCTION()
	void OnClimbMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// -------------------------------------- Climb Hop ------------------------------------------------
public:
	void HandleHop(float SafeDistance, int Index);
	bool CheckCanHopUp(float TraceDistance, float EyeOffset, float HopEndOffset);
	void RequestHopping();
	bool CanClimbUP();
	bool CanClimbDown();

	
private:
	bool IsServer() const;
	float CapR() const;
	float CapHH() const;
	
public:
	// --------------------------------------- Saved Mode -------------------------------------------------
	//! SaveMove snapshot
	class FSavedmove_Tales : public FSavedMove_Character
	{
		typedef  FSavedMove_Character Super;
	public:
		enum CompressedFlag
		{
			FLAG_Sprint    = 0x10,
			FLAG_Slide     = 0x20,
			FLAG_Climb     = 0x40,
			FLAG_Custom_3  = 0x80,
		};
		// :用来说明这个值只占一位
		//! CompressedFlags
		uint8 Saved_bWantsToSprint : 1;
		uint8 Saved_bPressedZippyJump : 1;
		uint8 Saved_bWantsToClimb : 1;
		uint8 Saved_bWantsToSlide : 1;

		//! To Save Some Variables
		uint8 Saved_bHadAnimRootMotion : 1;
		uint8 Saved_bTransitionFinished : 1;
		uint8 Saved_bPrevWantsToCrouch : 1;
		uint8 Saved_bWantsToProne      : 1;
		uint8 Saved_bClimbTransitionFinished : 1;

		FSavedmove_Tales();
		virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
		virtual void Clear() override;
		virtual uint8 GetCompressedFlags() const override;
		virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override;
		virtual void PrepMoveFor(ACharacter* C) override;
	};
	
	// --------------------------------------- Network Prediction Data -------------------------------------------------
	class FNetworkPredictionData_Client_Tales : public FNetworkPredictionData_Client_Character
	{
	public:
		FNetworkPredictionData_Client_Tales(const UCharacterMovementComponent& ClientMovement);
		typedef FNetworkPredictionData_Client_Character Super;
		virtual FSavedMovePtr AllocateNewMove() override;
	};
	
public:
	// --------------------------------------- Trigger Data -------------------------------------------------
	// @TODO This Can Move to GAS
	// 这两个有用, 作为TalesCharacter的接口了, 命名不好
	UFUNCTION(BlueprintCallable)
	void SprintPressed();
	UFUNCTION(BlueprintCallable)
	void SprintReleased();
	UFUNCTION(BlueprintCallable)
	void TalesInSlide();
	UFUNCTION(BlueprintCallable)
	void TalesOutSlide();
	UFUNCTION(BlueprintCallable)
	void TalesInProne();
	UFUNCTION(BlueprintCallable)
	void TalesOutProne();
};



