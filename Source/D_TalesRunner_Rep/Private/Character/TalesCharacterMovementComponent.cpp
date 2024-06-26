﻿
#include "Character/TalesCharacterMovementComponent.h"

#include <string>

#include "Character/TalesCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"

// Helper Macros
#if 0
float MacroDuration = 2.f;
#define SLOG(x) GEngine->AddOnScreenDebugMessage(-1, MacroDuration ? MacroDuration : -1.f, FColor::Yellow, x);
#define POINT(x, c) DrawDebugPoint(GetWorld(), x, 10, c, !MacroDuration, MacroDuration);
#define LINE(x1, x2, c) DrawDebugLine(GetWorld(), x1, x2, c, !MacroDuration, MacroDuration);
#define CAPSULE(x, c) DrawDebugCapsule(GetWorld(), x, CapHH(), CapR(), FQuat::Identity, c, !MacroDuration, MacroDuration);
#else
#define SLOG(x)
#define POINT(x, c)
#define LINE(x1, x2, c)
#define CAPSULE(x, c)
#endif


// ///////////////////////////////////////////////////////////////////////////////////////////////////
// Savedmode_Tales
// ///////////////////////////////////////////////////////////////////////////////////////////////////
#pragma region SavedMove
UTalesCharacterMovementComponent::FSavedmove_Tales::FSavedmove_Tales()
{
	Saved_bWantsToSprint = 0;
	Saved_bWantsToProne = 0;
	Saved_bPrevWantsToCrouch = 0;
	Saved_bWantsToSlide = 0;
}

bool UTalesCharacterMovementComponent::FSavedmove_Tales::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const
{
	FSavedmove_Tales* NewTalesMove = static_cast<FSavedmove_Tales*>(NewMove.Get());
	if(Saved_bWantsToSprint != NewTalesMove->Saved_bWantsToSprint)
	{
		return false;
	}
	if(Saved_bWantsToClimb != NewTalesMove->Saved_bWantsToClimb)
	{
		return false;
	}
	if(Saved_bWantsToSlide != NewTalesMove->Saved_bWantsToSlide)
	{
		return false;
	}
	
	return FSavedMove_Character::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void UTalesCharacterMovementComponent::FSavedmove_Tales::Clear()
{
	FSavedMove_Character::Clear();
	Saved_bWantsToSprint = 0;
	Saved_bPressedZippyJump = 0;
	Saved_bWantsToClimb = 0;
	Saved_bWantsToSlide = 0;

	Saved_bHadAnimRootMotion = 0;
	Saved_bTransitionFinished = 0;
	
	Saved_bWantsToProne = 0;
	Saved_bPrevWantsToCrouch = 0;
	Saved_bClimbTransitionFinished = 0;
}

uint8 UTalesCharacterMovementComponent::FSavedmove_Tales::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();
	if(Saved_bWantsToSprint) Result |= FLAG_Sprint;
	if(Saved_bPressedZippyJump) Result |= FLAG_JumpPressed;
	if(Saved_bWantsToClimb)  Result |= FLAG_Climb;
	if(Saved_bWantsToSlide)  Result |= FLAG_Slide;

	return Result;
}

void UTalesCharacterMovementComponent::FSavedmove_Tales::SetMoveFor(ACharacter* C, float InDeltaTime,
	FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	FSavedMove_Character::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);
	UTalesCharacterMovementComponent* CharacterMovement = Cast<UTalesCharacterMovementComponent>(C->GetCharacterMovement());
	Saved_bWantsToSprint	  = CharacterMovement->Safe_WantToSprint;
	Saved_bWantsToClimb       = CharacterMovement->Safe_WantsToClimb;
	Saved_bPrevWantsToCrouch  = CharacterMovement->Safe_PrevWantsToCrouch;
	Saved_bWantsToProne       = CharacterMovement->Safe_WantsToProne;
	Saved_bPressedZippyJump	  = CharacterMovement->TalesCharacterOwner->bPressedTalesJump;
	Saved_bHadAnimRootMotion  = CharacterMovement->Safe_HadAnimRootMotion;
	Saved_bTransitionFinished = CharacterMovement->Safe_TransitionFinished;
	Saved_bClimbTransitionFinished = CharacterMovement->Safe_ClimbTransitionFinished;
	Saved_bWantsToSlide		  = CharacterMovement->Safe_WantsToSlide;
}

void UTalesCharacterMovementComponent::FSavedmove_Tales::PrepMoveFor(ACharacter* C)
{
	FSavedMove_Character::PrepMoveFor(C);

	UTalesCharacterMovementComponent* CharacterMovement = Cast<UTalesCharacterMovementComponent>(C->GetCharacterMovement());
	CharacterMovement->Safe_WantToSprint      = Saved_bWantsToSprint;
	CharacterMovement->Safe_WantsToClimb      = Saved_bWantsToClimb;
	CharacterMovement->Safe_WantsToSlide      = Saved_bWantsToSlide;
	CharacterMovement->Safe_PrevWantsToCrouch = Saved_bPrevWantsToCrouch;
	CharacterMovement->Safe_WantsToProne      = Saved_bWantsToProne;
	CharacterMovement->TalesCharacterOwner->bPressedTalesJump = Saved_bPressedZippyJump;

	CharacterMovement->Safe_TransitionFinished = Saved_bTransitionFinished;
	CharacterMovement->Safe_HadAnimRootMotion  = Saved_bHadAnimRootMotion;
	CharacterMovement->Safe_ClimbTransitionFinished = Saved_bClimbTransitionFinished;
}

#pragma endregion

// ///////////////////////////////////////////////////////////////////////////////////////////////////
// PredicationData_Client
// ///////////////////////////////////////////////////////////////////////////////////////////////////
#pragma region PredicationData
UTalesCharacterMovementComponent::FNetworkPredictionData_Client_Tales::FNetworkPredictionData_Client_Tales(
	const UCharacterMovementComponent& ClientMovement) : Super(ClientMovement)
{
}

FSavedMovePtr UTalesCharacterMovementComponent::FNetworkPredictionData_Client_Tales::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedmove_Tales());
}

#pragma endregion

// ///////////////////////////////////////////////////////////////////////////////////////////////////
// TalesCharacterMovementComponent
// ///////////////////////////////////////////////////////////////////////////////////////////////////
UTalesCharacterMovementComponent::UTalesCharacterMovementComponent()
{
	NavAgentProps.bCanCrouch = true;
	CapsuleHalfHeightCrouch = GetCrouchedHalfHeight();
}

void UTalesCharacterMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();
	TalesCharacterOwner = Cast<ATalesCharacter>(GetOwner());
}

void UTalesCharacterMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	// @Todo Delete
	// OwningPlayerAnimInstance = CharacterOwner->GetMesh()->GetAnimInstance();

	// if(OwningPlayerAnimInstance)
	// {
	// 	OwningPlayerAnimInstance->OnMontageBlendingOut.AddDynamic(this, &UTalesCharacterMovementComponent::OnClimbMontageEnded);
	// }
}

bool UTalesCharacterMovementComponent::IsCustomMovementMode(ECustomMovementMode InCustomMocementMode) const
{
	return MovementMode == MOVE_Custom && CustomMovementMode == InCustomMocementMode;
}

bool UTalesCharacterMovementComponent::IsMovementMode(EMovementMode InMovementMode) const
{
	return InMovementMode == MovementMode;
}

FVector UTalesCharacterMovementComponent::GetUnRotatedClimbVelocity() const
{
	return UKismetMathLibrary::Quat_UnrotateVector(UpdatedComponent->GetComponentQuat(), Velocity);
}

float UTalesCharacterMovementComponent::GetMaxSpeed() const
{
	// Set Sprint Max Speed
	if(IsMovementMode(MOVE_Walking) && Safe_WantToSprint && !IsCrouching()) return MaxSprintSpeed * VelocityZoom;

	// Set Other Movement mode's Max Speed
	if(MovementMode != MOVE_Custom) return Super::GetMaxSpeed() * VelocityZoom;
	switch (CustomMovementMode)
	{
	case CMOVE_Slide:
		return MaxSlideSpeed * VelocityZoom;
	case CMOVE_Prone:
		return ProneMaxSpeed * VelocityZoom;
	case CMOVE_Climb:
		return MaxClimbSpeed * VelocityZoom;
	default:
		UE_LOG(LogTemp, Fatal, TEXT("Invalid Movement Mode"))
		return  -1.f;
	}
}

float UTalesCharacterMovementComponent::GetMaxBrakingDeceleration() const
{
	if(MovementMode != MOVE_Custom) return Super::GetMaxBrakingDeceleration();
	
	switch (CustomMovementMode)
	{
	case CMOVE_Slide:
		return BrakingDecelerationSliding;
	case CMOVE_Prone:
		return BrakingDecelerationProne;
	case CMOVE_Climb:
		return BrakingDecelerationClimbing;
	default:
		UE_LOG(LogTemp, Fatal, TEXT("Invalid Movement Mode"))
		return  -1.f;
	}
}

bool UTalesCharacterMovementComponent::IsMovingOnGround() const
{
	return Super::IsMovingOnGround() || IsCustomMovementMode(CMOVE_Slide) || IsCustomMovementMode(CMOVE_Prone);
}

bool UTalesCharacterMovementComponent::CanCrouchInCurrentState() const
{
	return Super::CanCrouchInCurrentState() && IsMovingOnGround();
}

FNetworkPredictionData_Client* UTalesCharacterMovementComponent::GetPredictionData_Client() const
{
	check(PawnOwner != nullptr);
	if(ClientPredictionData == nullptr)
	{
		UTalesCharacterMovementComponent* MutableThis = const_cast<UTalesCharacterMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Tales(*this);
		MutableThis->ClientPredictionData->MaxSmoothNetUpdateDist = 92.f;
		MutableThis->ClientPredictionData->NoSmoothNetUpdateDist = 140.f;
	}	
	return ClientPredictionData;
}

void UTalesCharacterMovementComponent::SimulateMovement(float DeltaTime)
{
	if(bHasReplicatedAcceleration)
	{
		const FVector OriginalAcceleration = Acceleration;
		Super::SimulateMovement(DeltaTime);
		Acceleration = OriginalAcceleration;
		LastUpdateAcceleration = Acceleration;
	}
	else
	{
		Super::SimulateMovement(DeltaTime);
	}
}

void UTalesCharacterMovementComponent::PerformMovement(float DeltaTime)
{
	Super::PerformMovement(DeltaTime);
	LastUpdateAcceleration = Acceleration;
}

// ----------------------------   Extended Character Movement Component   ------------------------------------
#pragma region ExtendedComp
void UTalesCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	Safe_WantToSprint = (Flags & FSavedmove_Tales::FLAG_Sprint) != 0;
	Safe_WantsToClimb = ((Flags & FSavedmove_Tales::FLAG_Climb) != 0);
	Safe_WantsToSlide = ((Flags & FSavedmove_Tales::FLAG_Slide) != 0);
}

void UTalesCharacterMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation,
                                                         const FVector& OldVelocity)
{
	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);

	// Safe_PrevWantsToCrouch = bWantsToCrouch;
}

void UTalesCharacterMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	// Climb
	// 但是: 为了避免if 后面的语句重复执行, 有关bWantsToCrouch的语句全部写在一个If-Else里面
	// @TODO 弃用
	// if(CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)
	// {
	// 	if(Safe_WantsToClimb && (MovementMode == MOVE_Walking || MovementMode == MOVE_Falling))
	// 	{
	// 		if(!Safe_ClimbTransitionFinished && (CanClimbUP() || CanClimbDown()))
	// 		{
	// 			Safe_WantsToClimb = false;
	// 		}
	// 		else if(Safe_ClimbTransitionFinished)
	// 		{
	// 			SetMovementMode(MOVE_Custom, CMOVE_Climb);
	// 		}
	// 		else
	// 		{
	// 			// 用来解决CanClimb()失败的情况
	// 			Safe_WantsToClimb = false;
	// 		}
	// 	}
	// 	else if(IsCustomMovementMode(CMOVE_Climb) && !Safe_WantsToClimb)
	// 	{
	// 		SetMovementMode(MOVE_Falling);
	// 	}
	// }

	//! Slide和Prone占用SlideFlags and Crouch 两个标志位
	// Slide  11 表示Slide
	if(CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)
	{
		if(MovementMode == MOVE_Walking && !bWantsToCrouch && Safe_WantsToSlide)
		{
			if(CanSlide())
			{
				SetMovementMode(MOVE_Custom, CMOVE_Slide);
			}
		}
	
		if(IsCustomMovementMode(CMOVE_Slide) && !bWantsToCrouch && !Safe_WantsToSlide)
		{
			SetMovementMode(MOVE_Walking);
		}


		// Prone
		if(MovementMode == MOVE_Walking && !bWantsToCrouch && Safe_WantsToSlide && Safe_WantsToClimb)
		{
			if(CanProne())
			{
				SetMovementMode(MOVE_Custom, CMOVE_Prone);
			
				// if(!CharacterOwner->HasAuthority()) Server_EnterProne();
			}
			// Safe_WantsToProne = false;
		}
		else if(IsCustomMovementMode(CMOVE_Prone) && !bWantsToCrouch && !Safe_WantsToSlide && !Safe_WantsToClimb)
		{
			SetMovementMode(MOVE_Walking);
		}

		// Mantle
		if(TalesCharacterOwner->bPressedTalesJump)
		{
			if(TryMantle())
			{
				TalesCharacterOwner->StopJumping();
			}
			else
			{
				SLOG("Failed Mantle, Reverting to jump")
				TalesCharacterOwner->bPressedTalesJump = false;
				CharacterOwner->bPressedJump = true;
				CharacterOwner->CheckJumpInput(DeltaSeconds);
			}
		}

		// Transition Mantle
		if(Safe_TransitionFinished)
		{
			SLOG("Transition Finished")
			UE_LOG(LogTemp, Warning, TEXT("FINISHED RM"))

			if(IsValid(TransitionQueuedMontage))
			{
				SetMovementMode(MOVE_Flying);
				CharacterOwner->PlayAnimMontage(TransitionQueuedMontage, TransitionQueuedMontageSpeed);
				TransitionQueuedMontageSpeed = 0.f;
				TransitionQueuedMontage = nullptr;
			}
			else
			{
				SetMovementMode(MOVE_Walking);
			}

			Safe_TransitionFinished = false;
		}
	}
	
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
}

void UTalesCharacterMovementComponent::UpdateCharacterStateAfterMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateAfterMovement(DeltaSeconds);

	if(!HasAnimRootMotion() && Safe_HadAnimRootMotion && IsMovementMode(MOVE_Flying))
	{
		if(Safe_HadAnimRootMotion)
		{
			UE_LOG(LogTemp, Warning, TEXT("Ending Anim Root Motion"))
			SetMovementMode(MOVE_Walking);
		}else if(MontageName == "ClimbOut")
		{
			SLOG("Has Quit Climb out Montage");
			StopMovementImmediately();
			SetMovementMode(MOVE_Walking);
		}
	}

	if(GetRootMotionSourceByID(TransitionRMS_ID) && GetRootMotionSourceByID(TransitionRMS_ID)->Status.HasFlag(ERootMotionSourceStatusFlags::Finished))
	{
		RemoveRootMotionSourceByID(TransitionRMS_ID);
		Safe_TransitionFinished = true;
	}
	Safe_HadAnimRootMotion = HasAnimRootMotion();
}

void UTalesCharacterMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	// Super::StartNewPhysics()
	Super::PhysCustom(deltaTime, Iterations);

	switch (CustomMovementMode)
	{
	case CMOVE_Slide:
		PhysSlide(deltaTime, Iterations);
		break;
	case CMOVE_Prone:
		PhysProne(deltaTime, Iterations);
		break;
	case CMOVE_Climb:
		PhysClimb(deltaTime, Iterations);
		break;
	default:
		UE_LOG(LogTemp, Fatal, TEXT("Invalid Movement Mode"))
	}
}

void UTalesCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
	// 之前的movement mode退出
	if(PreviousMovementMode == MOVE_Custom && PreviousCustomMode == CMOVE_Slide) ExitSlide();
	if(PreviousMovementMode == MOVE_Custom && PreviousCustomMode == CMOVE_Prone) ExitProne();
	if(PreviousMovementMode == MOVE_Custom && PreviousCustomMode == CMOVE_Climb) ExitClimb();

	// 进入现在的movement mode
	if(IsCustomMovementMode(CMOVE_Slide)) EnterSlide(PreviousMovementMode, (ECustomMovementMode)PreviousCustomMode);
	if(IsCustomMovementMode(CMOVE_Prone)) EnterProne(PreviousMovementMode, (ECustomMovementMode)PreviousCustomMode);
	if(IsCustomMovementMode(CMOVE_Climb)) EnterClimb(PreviousMovementMode, (ECustomMovementMode)PreviousCustomMode);
	
}
#pragma endregion

// -------------------------------------         Replication          -----------------------------------------
#pragma region Replication
void UTalesCharacterMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UTalesCharacterMovementComponent, Proxy_bShortMantle, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(UTalesCharacterMovementComponent, Proxy_bTallMantle, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(UTalesCharacterMovementComponent, Proxy_bClimbStart, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(UTalesCharacterMovementComponent, Safe_WantToSprint, COND_SimulatedOnly);
}

void UTalesCharacterMovementComponent::OnRep_ShortMantle()
{
	CharacterOwner->PlayAnimMontage(ProxyShortMantleMontage);
}

void UTalesCharacterMovementComponent::OnRep_TallMantle()
{
	CharacterOwner->PlayAnimMontage(ProxyTallMantleMontage);
}

void UTalesCharacterMovementComponent::OnRep_ClimbStart()
{
	CharacterOwner->PlayAnimMontage(ProxyClimbStartMontage);
}

void UTalesCharacterMovementComponent::OnRep_Sprint()
{
}
#pragma endregion

// ///////////////////////////////////////////////////////////////////////////////////////////////////////////
// ----------------------------------       Slide Movement Mode      ----------------------------------------
#pragma region Slide
void UTalesCharacterMovementComponent::EnterSlide(EMovementMode PrevMode, ECustomMovementMode PrevCustomMode)
{
	// bWantsToCrouch = true;
	bOrientRotationToMovement = false;
	// bUseControllerDesiredRotation = false;
	Velocity += Velocity.GetSafeNormal2D() * SlideEnterImpulse;

	FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, true, NULL);
}

bool UTalesCharacterMovementComponent::CanSlide() const
{
	FVector Start = UpdatedComponent->GetComponentLocation();
	FVector End = Start + CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2.5f * FVector::DownVector;
	FName ProfileName = TEXT("BlockAll");
	bool bValidSurface = GetWorld()->LineTraceTestByProfile(Start, End, ProfileName, TalesCharacterOwner->GetIgnoreCharacterParams());
	bool bEnoughSpeed = Velocity.SizeSquared() > pow(MinSlideSpeed, 2);
	return bValidSurface && bEnoughSpeed;
}

void UTalesCharacterMovementComponent::ExitSlide()
{
	bWantsToCrouch = false;
	bOrientRotationToMovement = true;
	TalesOutSlide();
	// bUseControllerDesiredRotation = true;
}

void UTalesCharacterMovementComponent::PhysSlide(float deltaTime, int32 Iterations)
{
	if(deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	if(!CanSlide())
	{
		ExitSlide();
		SetMovementMode(MOVE_Walking);
		StartNewPhysics(deltaTime, Iterations);
		return;
	}
	
	bJustTeleported = false;
	bool bCheckedFall = false;
	bool bTriedLedgeMove =false;
	float remainingTime = deltaTime;

	// Perform the move
	while((
		remainingTime >= MIN_TICK_TIME) &&
		(Iterations < MaxSimulationIterations) &&
		CharacterOwner &&
		(
			CharacterOwner->Controller ||
			bRunPhysicsWithNoController ||
			(CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy)))
	{
		Iterations++;
		bJustTeleported = false;
		const float timeTick = GetSimulationTimeStep(remainingTime, Iterations);
		remainingTime -= timeTick;

		// Save Current values
		UPrimitiveComponent* const OldBase = GetMovementBase();
		const FVector PreviousBaseLocation = (OldBase != NULL) ? OldBase->GetComponentLocation() : FVector::ZeroVector;
		const FVector OldLocation = UpdatedComponent->GetComponentLocation();
		const FFindFloorResult OldFloor = CurrentFloor;

		// Ensure velocity is horizontal
		MaintainHorizontalGroundVelocity();
		const FVector OldVelocity = Velocity;

		FVector SlopeForce = CurrentFloor.HitResult.Normal;
		SlopeForce.Z = 0.f;
		Velocity += SlopeForce * SlideGravityForce * deltaTime;
		float BrakingDeceleration = 0;
		if(Acceleration.IsNearlyZero() || FVector::DotProduct(Acceleration, Velocity) < 0)
		{
			BrakingDeceleration = GetMaxBrakingDeceleration();
			Acceleration = FVector::ZeroVector;
		}
		else
		{
			Acceleration = Acceleration.ProjectOnTo(UpdatedComponent->GetRightVector().GetSafeNormal2D());
		}
		// Apply acceleration
		CalcVelocity(timeTick, GroundFriction * SlideFriction, true, BrakingDeceleration);

		// Compute move parameters
		const FVector MoveVelocity = Velocity;
		const FVector Delta = timeTick * MoveVelocity;
		const bool bZeroDelta = Delta.IsNearlyZero();
		FStepDownResult StepDownResult;
		bool bFloorWalkable = CurrentFloor.IsWalkableFloor();
		if(bZeroDelta)
		{
			remainingTime = 0.f;	
		}
		else
		{
			// Try to move forward
			MoveAlongFloor(MoveVelocity, timeTick, &StepDownResult);

			if(IsFalling())
			{
				// Pawn Decided to jump up
				const float DesiredDist = Delta.Size();
				if(DesiredDist > KINDA_SMALL_NUMBER)
				{
					const float ActualDist = (UpdatedComponent->GetComponentLocation() - OldLocation).Size2D();
					remainingTime += timeTick * (1.f - FMath::Min(1.f, ActualDist / DesiredDist));
				}
				StartNewPhysics(remainingTime, Iterations);
				return;
			}
			else if(IsSwimming()) // Just entered water
			{
				StartSwimming(OldLocation, OldVelocity, timeTick, remainingTime, Iterations);
				return;
			}
		}

		//Update floor
		// StepUp might have already done it for us
		if(StepDownResult.bComputedFloor)
		{
			CurrentFloor = StepDownResult.FloorResult;
		}
		else
		{
			FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, bZeroDelta, NULL);
		}

		// Check for ledges here
		const bool bChechLedges = !CanWalkOffLedges();
		if(bChechLedges && !CurrentFloor.IsWalkableFloor())
		{
			// calculate possible alternate movement
			const FVector GravDir = FVector(0.f, 0.f, -1.f);
			const FVector NewDelta = bTriedLedgeMove ? FVector::ZeroVector : GetLedgeMove(OldLocation, Delta, GravDir);
			if(!NewDelta.IsZero())
			{
				// First revert this move
				RevertMove(OldLocation, OldBase, PreviousBaseLocation, OldFloor, false);

				// avoid repeated ledge moves if the first one fails
				bTriedLedgeMove = true;
				Velocity = NewDelta / timeTick;
				remainingTime += timeTick;
				continue;
			}
			else
			{
				// see if it is OK to jump
				// @todo collision : only thing that can be problem is that oldbase has world collision on
				bool bMustJump = bZeroDelta || (OldBase == NULL || (!OldBase->IsQueryCollisionEnabled() && MovementBaseUtility::IsDynamicBase(OldBase)));
				if ( (bMustJump || !bCheckedFall) && CheckFall(OldFloor, CurrentFloor.HitResult, Delta, OldLocation, remainingTime, timeTick, Iterations, bMustJump) )
				{
					return;
				}
				bCheckedFall = true;

				// revert this move
				RevertMove(OldLocation, OldBase, PreviousBaseLocation, OldFloor, true);
				remainingTime = 0.f;
				break;
			}
		}
		else
		{
			// Validate the floor check
			if (CurrentFloor.IsWalkableFloor())
			{
				if (ShouldCatchAir(OldFloor, CurrentFloor))
				{
					HandleWalkingOffLedge(OldFloor.HitResult.ImpactNormal, OldFloor.HitResult.Normal, OldLocation, timeTick);
					if (IsMovingOnGround())
					{
						// If still walking, then fall. If not, assume the user set a different mode they want to keep.
						StartFalling(Iterations, remainingTime, timeTick, Delta, OldLocation);
					}
					return;
				}

				AdjustFloorHeight();
				SetBase(CurrentFloor.HitResult.Component.Get(), CurrentFloor.HitResult.BoneName);
			}
			else if (CurrentFloor.HitResult.bStartPenetrating && remainingTime <= 0.f)
			{
				// The floor check failed because it started in penetration
				// We do not want to try to move downward because the downward sweep failed, rather we'd like to try to pop out of the floor.
				FHitResult Hit(CurrentFloor.HitResult);
				Hit.TraceEnd = Hit.TraceStart + FVector(0.f, 0.f, MAX_FLOOR_DIST);
				const FVector RequestedAdjustment = GetPenetrationAdjustment(Hit);
				ResolvePenetration(RequestedAdjustment, Hit, UpdatedComponent->GetComponentQuat());
				bForceNextFloorCheck = true;
			}
			
			// check if just entered water
			if ( IsSwimming() )
			{
				StartSwimming(OldLocation, Velocity, timeTick, remainingTime, Iterations);
				return;
			}

			// See if we need to start falling.
			if (!CurrentFloor.IsWalkableFloor() && !CurrentFloor.HitResult.bStartPenetrating)
			{
				const bool bMustJump = bJustTeleported || bZeroDelta || (OldBase == NULL || (!OldBase->IsQueryCollisionEnabled() && MovementBaseUtility::IsDynamicBase(OldBase)));
				if ((bMustJump || !bCheckedFall) && CheckFall(OldFloor, CurrentFloor.HitResult, Delta, OldLocation, remainingTime, timeTick, Iterations, bMustJump) )
				{
					return;
				}
				bCheckedFall = true;
			}
		}
		if(IsMovingOnGround() && bFloorWalkable)
		{
			if(!bJustTeleported && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() && timeTick >= MIN_TICK_TIME)
			{
				// TODO-RootMotionSource: Allow this to happen during partial override Velocity, but only set allowed axes?
				Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / timeTick;
				MaintainHorizontalGroundVelocity();
			}	
		}

		// If we didn't move at all this iteration then abort (since future iterations will also be stuck).
		if (UpdatedComponent->GetComponentLocation() == OldLocation)
		{
			remainingTime = 0.f;
			break;
		}	
	}

	FHitResult Hit;
	FQuat NewRotation = FRotationMatrix::MakeFromXZ(Velocity.GetSafeNormal2D(), FVector::UpVector).ToQuat();
	SafeMoveUpdatedComponent(FVector::ZeroVector, NewRotation, false, Hit);
}
#pragma endregion

// ///////////////////////////////////////////////////////////////////////////////////////////////////////////
// ----------------------------------       Prone Movement Mode      ----------------------------------------
#pragma region Prone
void UTalesCharacterMovementComponent::Server_EnterProne_Implementation()
{
	// Safe_WantsToProne = true;
}

void UTalesCharacterMovementComponent::EnterProne(EMovementMode PrevMode, ECustomMovementMode PrevCustomMode)
{
	bWantsToCrouch = true;
	if(PrevMode == MOVE_Custom && PrevCustomMode == CMOVE_Slide)
	{
		Velocity += Velocity.GetSafeNormal2D() * ProneSlideEnterImpulse;
	}
	FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, true, NULL);
}

bool UTalesCharacterMovementComponent::CanProne() const
{
	return IsCustomMovementMode(CMOVE_Slide) || IsMovementMode(MOVE_Walking);
}

void UTalesCharacterMovementComponent::ExitProne()
{
	bWantsToCrouch = false;
	TalesOutProne();
}

void UTalesCharacterMovementComponent::PhysProne(float deltaTime, int32 Iterations)
{
	if(deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	if(!CharacterOwner ||
		(
			!CharacterOwner->GetController() &&
			!bRunPhysicsWithNoController &&
			!HasAnimRootMotion() &&
			!CurrentRootMotion.HasOverrideVelocity() &&
			(CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)))
	{
		Acceleration = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
		return;
	}

	bJustTeleported = false;
	bool bCheckedFall = false;
	bool bTriedLedgeMove = false;
	float remainingTime = deltaTime;

	// Perform the move
	while(
		(remainingTime >= MIN_TICK_TIME) &&
		(Iterations < MaxSimulationIterations) &&
		CharacterOwner &&
		(
			CharacterOwner->GetController() ||
			bRunPhysicsWithNoController ||
			CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy))
	{
		Iterations++;
		bJustTeleported = false;
		const float timeTick = GetSimulationTimeStep(remainingTime, Iterations);
		remainingTime -= timeTick;
		
		// Save Current Values
		UPrimitiveComponent* const OldBase = GetMovementBase();
		const FVector PreviousBaseLocation = (OldBase != nullptr) ? OldBase->GetComponentLocation() : FVector::ZeroVector;
		const FVector OldLocation = UpdatedComponent->GetComponentLocation();
		const FFindFloorResult OldFloor = CurrentFloor;

		// Ensure Velocity is horizontal
		MaintainHorizontalGroundVelocity();
		const FVector OldVelocity = Velocity;
		Acceleration.Z = 0.f;

		// Apply Acceleration
		CalcVelocity(timeTick, GroundFriction, false, GetMaxBrakingDeceleration());

		// Compute move parameters
		const FVector MoveVelocity = Velocity;
		const FVector Delta = timeTick * MoveVelocity;
		const bool bZeroDelta = Delta.IsNearlyZero();
		FStepDownResult StepDownResult;

		if(bZeroDelta)
		{
			remainingTime = 0.f;
		}
		else
		{
			// Real movement 
			MoveAlongFloor(MoveVelocity, timeTick, &StepDownResult);

			if(IsFalling())
			{
				// Pawn decided to jump up
				const float DesiredDist = Delta.Size();
				if(DesiredDist > KINDA_SMALL_NUMBER)
				{
					const float ActualDist = (UpdatedComponent->GetComponentLocation() - OldLocation).Size2D();
					remainingTime += timeTick * (1.f - FMath::Min(1.f, ActualDist / DesiredDist));
				}
				StartNewPhysics(remainingTime, Iterations);
				return;
			}
			else if(IsSwimming()) // Just entered water
			{
				StartSwimming(OldLocation, OldVelocity, timeTick, remainingTime, Iterations);
				return;
			}
		}

		// Update floor
		// StepUp might have already done it for us.
		if (StepDownResult.bComputedFloor)
		{
			CurrentFloor = StepDownResult.FloorResult;
		}
		else
		{
			FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, bZeroDelta, NULL);
		}

		// check for ledges here
		const bool bCheckLedges = !CanWalkOffLedges();
		if ( bCheckLedges && !CurrentFloor.IsWalkableFloor() )
		{
			// calculate possible alternate movement
			const FVector GravDir = FVector(0.f,0.f,-1.f);
			const FVector NewDelta = bTriedLedgeMove ? FVector::ZeroVector : GetLedgeMove(OldLocation, Delta, GravDir);
			if ( !NewDelta.IsZero() )
			{
				// first revert this move
				RevertMove(OldLocation, OldBase, PreviousBaseLocation, OldFloor, false);

				// avoid repeated ledge moves if the first one fails
				bTriedLedgeMove = true;

				// Try new movement direction
				Velocity = NewDelta/timeTick;
				remainingTime += timeTick;
				continue;
			}
			else
			{
				// see if it is OK to jump
				// @todo collision : only thing that can be problem is that oldbase has world collision on
				bool bMustJump = bZeroDelta || (OldBase == NULL || (!OldBase->IsQueryCollisionEnabled() && MovementBaseUtility::IsDynamicBase(OldBase)));
				if ( (bMustJump || !bCheckedFall) && CheckFall(OldFloor, CurrentFloor.HitResult, Delta, OldLocation, remainingTime, timeTick, Iterations, bMustJump) )
				{
					return;
				}
				bCheckedFall = true;

				// revert this move
				RevertMove(OldLocation, OldBase, PreviousBaseLocation, OldFloor, true);
				remainingTime = 0.f;
				break;
			}
		}
		else
		{
			// Validate the floor check
			if (CurrentFloor.IsWalkableFloor())
			{
				AdjustFloorHeight();
				SetBase(CurrentFloor.HitResult.Component.Get(), CurrentFloor.HitResult.BoneName);
			}
			else if (CurrentFloor.HitResult.bStartPenetrating && remainingTime <= 0.f)
			{
				// The floor check failed because it started in penetration
				// We do not want to try to move downward because the downward sweep failed, rather we'd like to try to pop out of the floor.
				FHitResult Hit(CurrentFloor.HitResult);
				Hit.TraceEnd = Hit.TraceStart + FVector(0.f, 0.f, MAX_FLOOR_DIST);
				const FVector RequestedAdjustment = GetPenetrationAdjustment(Hit);
				ResolvePenetration(RequestedAdjustment, Hit, UpdatedComponent->GetComponentQuat());
				bForceNextFloorCheck = true;
			}

			// check if just entered water
			if ( IsSwimming() )
			{
				StartSwimming(OldLocation, Velocity, timeTick, remainingTime, Iterations);
				return;
			}

			// See if we need to start falling.
			if (!CurrentFloor.IsWalkableFloor() && !CurrentFloor.HitResult.bStartPenetrating)
			{
				const bool bMustJump = bJustTeleported || bZeroDelta || (OldBase == NULL || (!OldBase->IsQueryCollisionEnabled() && MovementBaseUtility::IsDynamicBase(OldBase)));
				if ((bMustJump || !bCheckedFall) && CheckFall(OldFloor, CurrentFloor.HitResult, Delta, OldLocation, remainingTime, timeTick, Iterations, bMustJump) )
				{
					return;
				}
				bCheckedFall = true;
			}
		}

		// Allow overlap events and such to change physics state and velocity
		if (IsMovingOnGround())
		{
			// Make velocity reflect actual move
			if( !bJustTeleported && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() && timeTick >= MIN_TICK_TIME)
			{
				// TODO-RootMotionSource: Allow this to happen during partial override Velocity, but only set allowed axes?
				Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / timeTick;
				MaintainHorizontalGroundVelocity();
			}
		}

		// If we didn't move at all this iteration then abort (since future iterations will also be stuck).
		if (UpdatedComponent->GetComponentLocation() == OldLocation)
		{
			remainingTime = 0.f;
			break;
		}	
	}

	if(IsMovingOnGround())
	{
		MaintainHorizontalGroundVelocity();
	}
}
#pragma endregion

// ---------------------------------------       Mantle        ---------------------------------------------
#pragma region Mantle
bool UTalesCharacterMovementComponent::TryMantle()
{
	// 如果现在不是 Fall 并且 不是Walk 直接退出(custom一般都不行, Slide && Prone), 或者不是Fall 是 Walk 但是Crouch
	if(!(IsMovementMode(MOVE_Walking) && !IsCrouching()) &&
		!IsMovementMode(MOVE_Falling))
	{
		return false;
	}

	// 辅助的变量
	FVector BaseLoc = UpdatedComponent->GetComponentLocation() + FVector::DownVector * CapHH();
	FVector ForwardVec = UpdatedComponent->GetForwardVector().GetSafeNormal2D();
	auto Params = TalesCharacterOwner->GetIgnoreCharacterParams();
	float MaxHeight = CapHH() * 2 + MantleReachHeight;
	float CosMMinWallSteepA = FMath::Cos(FMath::DegreesToRadians(MantleMinWallSteepnessAngle));
	float CosMMaxSurfaceA  = FMath::Cos(FMath::DegreesToRadians(MantleMaxSurfaceAngle));
	float CosMMaxAlignA  = FMath::Cos(FMath::DegreesToRadians(MantleMaxAlignmentAngle));

	SLOG("Start Mantle Attemp")
	
	// Check Front Face
	FHitResult FrontHit;
	float CheckDistance = FMath::Clamp(Velocity | ForwardVec, CapR() + 30, MantleMaxDistance);
	FVector FrontStart = BaseLoc + FVector::UpVector * (MaxStepHeight - 1);
	for(int i = 0; i < MantleScanNumber; ++i)
	{
		
		LINE(FrontStart, FrontStart + ForwardVec * CheckDistance, FColor::Red)
		
				if(GetWorld()->LineTraceSingleByProfile(FrontHit, FrontStart, FrontStart + ForwardVec * CheckDistance, "BlockAll", Params))
				{
					break;
				}
		FrontStart += FVector::UpVector * (2.f * CapHH() - (MaxStepHeight - 1)) / (MantleScanNumber - 1);	
	}

	if(!FrontHit.IsValidBlockingHit()) return false;
	float CosWallSteepnessAngle = FrontHit.Normal | FVector::UpVector; // Dot Product 重载操作符
	if(FMath::Abs(CosWallSteepnessAngle) > CosMMinWallSteepA || (ForwardVec | -FrontHit.Normal) < CosMMaxAlignA)
	{
		return false;
	}
	POINT(FrontHit.Location, FColor::Red);
	
	// Check Height
	TArray<FHitResult> HeightHits;
	FHitResult SurfaceHit;
	FVector WallUp = FVector::VectorPlaneProject(FVector::UpVector, FrontHit.Normal).GetSafeNormal();
	float WallCos = FVector::UpVector | FrontHit.Normal;
	float WallSin = FMath::Sqrt(1 - WallCos * WallCos);
	FVector TraceStart = FrontHit.Location + ForwardVec + WallUp * (MaxHeight - (MaxStepHeight - 1)) / WallSin;
	if(!GetWorld()->LineTraceMultiByProfile(HeightHits, TraceStart, FrontHit.Location + ForwardVec, "BlockAll", Params))
	{
		return false;
	}

	for(const FHitResult& Hit : HeightHits)
	{
		if(Hit.IsValidBlockingHit())
		{
			SurfaceHit = Hit;
			break;
		}
	}

	// 如果要爬上的表面太斜了 也不行
	if(!SurfaceHit.IsValidBlockingHit() || (SurfaceHit.Normal | FVector::UpVector) < CosMMaxSurfaceA)
	{
		return false;
	}

	float Height = (SurfaceHit.Location - BaseLoc) | FVector::UpVector;
	
	SLOG(FString::Printf(TEXT("Height: %f"), Height))
	POINT(SurfaceHit.Location, FColor::Blue);
	
	// 判断是否可以跳上去
	if(Height > MaxHeight) return false;

	// 判断这个surface表面能不能站上一个胶囊体
	float SurfaceCos = FVector::UpVector | SurfaceHit.Normal;
	float SurfaceSin = FMath::Sqrt(1 - SurfaceCos * SurfaceCos);
	FVector ClearCapLoc = SurfaceHit.Location + ForwardVec * CapR() + FVector::UpVector * (CapHH() + 1 + CapR() * 2 * SurfaceSin);
	FCollisionShape CapShape = FCollisionShape::MakeCapsule(CapR(), CapHH());
	if(GetWorld()->OverlapAnyTestByProfile(ClearCapLoc, FQuat::Identity, "BlockAll", CapShape, Params))
	{
		CAPSULE(ClearCapLoc, FColor::Red)
				return false;
	}
	else
	{
		CAPSULE(ClearCapLoc, FColor::Green)
	}
SLOG("Can Mantle")	

	// Check Clearance. 因为用的是根骨骼动画, 需要计算过度动画开始的时间
	// Check Depth
	// Mantle Selection
	FVector ShortMantleTarget = GetMantleStartLocation(FrontHit, SurfaceHit, false);
	FVector TallMantleTarget = GetMantleStartLocation(FrontHit, SurfaceHit, true);

	bool bTallMantle = false;
	if(IsMovementMode(MOVE_Walking) && Height > CapHH() * 2)
	{
		bTallMantle = true;
	}
	else if(IsMovementMode(MOVE_Flying) && (Velocity | FVector::UpVector) < 0)
	{
		// 看看还能不能下降了
		if(!GetWorld()->OverlapAnyTestByProfile(TallMantleTarget, FQuat::Identity, "BlockAll", CapShape, Params))
		{
			bTallMantle = true;
		}
	}
	FVector TransitionTarget = bTallMantle ? TallMantleTarget : ShortMantleTarget;
	
CAPSULE(TransitionTarget, FColor::Yellow)

	// Perform Transition to Mantle
CAPSULE(UpdatedComponent->GetComponentLocation(), FColor::Red);

	float UpSpeed = Velocity | FVector::UpVector;
	float TransDistance = FVector::Dist(TransitionTarget, UpdatedComponent->GetComponentLocation());

	TransitionQueuedMontageSpeed = FMath::GetMappedRangeValueClamped(FVector2D(-500, 750), FVector2D(.9f, 1.2f), UpSpeed);
	TransitionRMS.Reset();
	TransitionRMS = MakeShared<FRootMotionSource_MoveToForce>();
	TransitionRMS->AccumulateMode = ERootMotionAccumulateMode::Override;

	TransitionRMS->Duration = FMath::Clamp(TransDistance / 500.f, .1f, .25f);
	
SLOG(FString::Printf(TEXT("Duration: %f"), TransitionRMS->Duration))
	TransitionRMS->StartLocation = UpdatedComponent->GetComponentLocation();
	TransitionRMS->TargetLocation = TransitionTarget;

	// Apply Transition Root Motion Source
	Velocity = FVector::ZeroVector;
	SetMovementMode(MOVE_Flying);
	TransitionRMS_ID = ApplyRootMotionSource(TransitionRMS);

	// Animations
	if(bTallMantle)
	{
		TransitionQueuedMontage = TallMantleMontage;
		CharacterOwner->PlayAnimMontage(TransitionTallMantleMontage, 1 / TransitionRMS->Duration);
		if(IsServer()) Proxy_bTallMantle = !Proxy_bTallMantle;
	}
	else
	{
		TransitionQueuedMontage = ShortMantleMontage;
		CharacterOwner->PlayAnimMontage(TransitionShortMantleMontage, 1 / TransitionRMS->Duration);
		if(IsServer()) Proxy_bShortMantle = !Proxy_bShortMantle;
	}
	return true;	
}

FVector UTalesCharacterMovementComponent::GetMantleStartLocation(FHitResult FrontHit, FHitResult SurfaceHit,
	bool bTallMantle) const
{
	float CosWallSteepnessAngle = FrontHit.Normal | FVector::UpVector;
	float DownDistance = bTallMantle ? CapHH() * 2.f : MaxStepHeight - 1;
	FVector EdgeTangent = FVector::CrossProduct(SurfaceHit.Normal, FrontHit.Normal).GetSafeNormal();

	FVector MantleStart = SurfaceHit.Location;
	MantleStart += FrontHit.Normal.GetSafeNormal2D() * (2.f + CapR());
	MantleStart += UpdatedComponent->GetForwardVector().GetSafeNormal2D().ProjectOnTo(EdgeTangent) * CapR();
	MantleStart += FVector::UpVector * CapHH();
	MantleStart += FVector::DownVector * DownDistance;
	MantleStart += FrontHit.Normal.GetSafeNormal2D() * CosWallSteepnessAngle * DownDistance;

	return MantleStart;
}

bool UTalesCharacterMovementComponent::IsServer() const
{
	return CharacterOwner->HasAuthority();
}

float UTalesCharacterMovementComponent::CapR() const
{
	return CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius();
}

float UTalesCharacterMovementComponent::CapHH() const
{
	return CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
}
#pragma endregion


// ///////////////////////////////////////////////////////////////////////////////////////////////////////////
// ----------------------------------       Climb Movement Mode      ----------------------------------------
#pragma region Climb
void UTalesCharacterMovementComponent::EnterClimb(EMovementMode PrevMode, ECustomMovementMode PrevCustomMode)
{
	SLOG("Start Climbing");
	Velocity = FVector::Zero();
	bUseControllerDesiredRotation =false;
	bOrientRotationToMovement = false;
	CharacterOwner->bUseControllerRotationYaw = false;

	// 用来切换Input Mapping
	OnEnterClimbStateDelegate.ExecuteIfBound();
}

void UTalesCharacterMovementComponent::ExitClimb()
{
	Safe_WantsToClimb = false;
	Safe_ClimbTransitionFinished = false;
	bUseControllerDesiredRotation = true;
	bOrientRotationToMovement = true;
	CharacterOwner->bUseControllerRotationYaw = true;
	// 用来切换Input Mapping
	OnExitClimbStateDelegate.ExecuteIfBound();

	FHitResult Hit;
	const FRotator DirtyRotation = UpdatedComponent->GetComponentRotation();
	const FQuat CleanStandRotation = FRotator(0.f, DirtyRotation.Yaw, 0.f).Quaternion();
	SafeMoveUpdatedComponent(FVector::ZeroVector, CleanStandRotation, false, Hit);
}

bool UTalesCharacterMovementComponent::CanClimbUP()
{
	// TraceClimbableSurfaces()
	// 这里是有一些问题的
	const FVector StartOffset = UpdatedComponent->GetForwardVector() * (CapR() - 1.f);
	const FVector Start = UpdatedComponent->GetComponentLocation() + StartOffset;
	const FVector End = Start + UpdatedComponent->GetForwardVector();
	TArray<FHitResult> CapsuleTraceHitResults;
	FCollisionShape CapShape = FCollisionShape::MakeCapsule(CapR() * 1.2, CapHH());
	if(!GetWorld()->SweepMultiByProfile(CapsuleTraceHitResults, Start, End, FQuat::Identity , "BlockAll", CapShape, TalesCharacterOwner->GetIgnoreCharacterParams()))
	{
CAPSULE(Start, FColor::Red)
		return false;
	}

	// UseLineTrace to Search high
	FHitResult FrontHit;
	float TraceStartOffset = 30.f;
	const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
	const FVector EyeHeightOffset = UpdatedComponent->GetUpVector() * (CharacterOwner->BaseEyeHeight + TraceStartOffset);
	const FVector LineStart = ComponentLocation + EyeHeightOffset;
	if(!GetWorld()->LineTraceSingleByProfile(FrontHit, LineStart, LineStart + UpdatedComponent->GetForwardVector() * ClimbReachDistance, "BlockAll", TalesCharacterOwner->GetIgnoreCharacterParams()))
	{
LINE(LineStart, LineStart + UpdatedComponent->GetForwardVector() * ClimbReachDistance, FColor::Red);
		return false;
	}
	if(!FrontHit.IsValidBlockingHit()) return false;

	
	FHitResult Hit;
	const FRotator DistanceRotation = UKismetMathLibrary::MakeRotFromX(-FrontHit.Normal);
	SafeMoveUpdatedComponent(FVector::ZeroVector, DistanceRotation.Quaternion(), false, Hit);
	// 改用Ability
	if(TransitionClimbUpMontage && CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)
	{
		// @Todo 弃用
		OwningPlayerAnimInstance->Montage_Play(TransitionClimbUpMontage);
		if(IsServer())
		{
			// Proxy_bClimbStart = !Proxy_bClimbStart;
		}
	}
	return true;
}

bool UTalesCharacterMovementComponent::CanClimbDown()
{
	// LineTrace To Get Normal
	if(IsFalling()) return false;
	
	FVector WalkableSurfaceTraceStart = UpdatedComponent->GetComponentLocation() + UpdatedComponent->GetForwardVector() * ClimbDownWalkableSurfaceTraceOffset;
	FVector WalkableSurfaceTraceEnd = WalkableSurfaceTraceStart - UpdatedComponent->GetUpVector() * 100.f;
	FHitResult WalkableSurfaceHit;
	GetWorld()->LineTraceSingleByProfile(WalkableSurfaceHit, WalkableSurfaceTraceStart, WalkableSurfaceTraceEnd, "BlockAll", TalesCharacterOwner->GetIgnoreCharacterParams());
	LINE(WalkableSurfaceTraceStart, WalkableSurfaceTraceEnd, FColor::Blue);

	WalkableSurfaceTraceStart = WalkableSurfaceHit.TraceStart + UpdatedComponent->GetForwardVector() * ClimbDownLedgeTraceOffset;
	WalkableSurfaceTraceEnd = WalkableSurfaceTraceStart - UpdatedComponent->GetUpVector() * CapHH() * 4.2f;
	FHitResult LedgeTraceHit;
	GetWorld()->LineTraceSingleByProfile(LedgeTraceHit, WalkableSurfaceTraceStart, WalkableSurfaceTraceEnd, "BlockAll", TalesCharacterOwner->GetIgnoreCharacterParams());
	LINE(WalkableSurfaceTraceStart, WalkableSurfaceTraceEnd, FColor::Red);
	if(WalkableSurfaceHit.bBlockingHit && !LedgeTraceHit.bBlockingHit)
	{
		if(CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)
		{
			// SafeMoveUpdatedComponent(UpdatedComponent->GetForwardVector() * ClimbDownWalkableSurfaceTraceOffset, UpdatedComponent->GetComponentRotation(), false, WalkableSurfaceHit);
		}
		// @TODO 现在不会调用了
		if(TransitionClimbDownMontage && CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)
		{
			OwningPlayerAnimInstance->Montage_Play(TransitionClimbDownMontage);
			if(IsServer())
			{
				// Proxy_bClimbStart = !Proxy_bClimbStart;
			}
		}
		return true;
	}
	return false;	
}

void UTalesCharacterMovementComponent::PhysClimb(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}
	if (!CharacterOwner || (!CharacterOwner->Controller && !bRunPhysicsWithNoController && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() && (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)))
	{
		Acceleration = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
		return;
	}

	// Perform the move
	bJustTeleported = false;
	Iterations++;
	
	FVector StartOffset = -1 * UpdatedComponent->GetUpVector();
	FVector Start = UpdatedComponent->GetComponentLocation() + StartOffset * 50.f;
	FVector End = Start + StartOffset;
	TArray<FHitResult> CapsuleTraceHitResults;
	FCollisionShape CapShape = FCollisionShape::MakeCapsule(50.f, 72.f);
	GetWorld()->SweepMultiByProfile(CapsuleTraceHitResults, Start, End, FQuat::Identity, "BlockAll", CapShape, TalesCharacterOwner->GetIgnoreCharacterParams());
	if(!CapsuleTraceHitResults.IsEmpty())
	{
		for(auto& Hit : CapsuleTraceHitResults)
		{
			if(GetUnRotatedClimbVelocity().Z < -10.f && FVector::Parallel(Hit.ImpactNormal, FVector::UpVector))
			{
				SetMovementMode(MOVE_Falling);
				StartNewPhysics(deltaTime, Iterations);
				return;
			}
		}
	}

	// Check has reached floor
	StartOffset = UpdatedComponent->GetForwardVector() * 30.f;
	Start = UpdatedComponent->GetComponentLocation() + StartOffset;
	End = Start + UpdatedComponent->GetForwardVector();
	CapsuleTraceHitResults.Reset();
	GetWorld()->SweepMultiByProfile(CapsuleTraceHitResults, Start, End, FQuat::Identity , "BlockAll", CapShape, TalesCharacterOwner->GetIgnoreCharacterParams());
	if(CapsuleTraceHitResults.IsEmpty())
	{
		SetMovementMode(MOVE_Falling);
		StartNewPhysics(deltaTime, Iterations);
		return;
	}
	
	// ProcessClimableSurfaceInfo
	CurrentClimbableSurfaceLocation = FVector::ZeroVector;
	CurrentClimbablefaceNormal = FVector::ZeroVector;
	for(const FHitResult& TraceHitResult : CapsuleTraceHitResults)
	{
		CurrentClimbableSurfaceLocation += TraceHitResult.ImpactPoint;
		CurrentClimbablefaceNormal += TraceHitResult.ImpactNormal;
	}
	
	CurrentClimbableSurfaceLocation /= CapsuleTraceHitResults.Num();
	CurrentClimbablefaceNormal = CurrentClimbablefaceNormal.GetSafeNormal();

	// Check we should exit Climb
	const float DotResult = FVector::DotProduct(CurrentClimbablefaceNormal, FVector::UpVector);

	if(DotResult >= FMath::Cos(PI / 3.0))
	{
		SetMovementMode(MOVE_Falling);
		StartNewPhysics(deltaTime, Iterations);
		return;
	}

	// Check Has Reached Ledge
	FHitResult LedgetHitResult;
	float TraceStartOffset = 30.f;
	const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
	const FVector EyeHeightOffset = UpdatedComponent->GetUpVector() * (CharacterOwner->BaseEyeHeight + TraceStartOffset);
	const FVector LineStart = ComponentLocation + EyeHeightOffset;
	// LINE(LineStart, LineStart + UpdatedComponent->GetUpVector() * 100.f, FColor::Yellow);
	GetWorld()->LineTraceSingleByProfile(LedgetHitResult, LineStart, LineStart + UpdatedComponent->GetForwardVector() * 100.f, "BlockAll", TalesCharacterOwner->GetIgnoreCharacterParams());
	// LINE(LineStart, LineStart + UpdatedComponent->GetForwardVector() * 150.f, FColor::Red);
	if(!LedgetHitResult.bBlockingHit)
	{
		const FVector WalkableSurfaceTraceStart = LedgetHitResult.TraceEnd;
		const FVector WalkableSurfaceTraceEnd = WalkableSurfaceTraceStart - UpdatedComponent->GetUpVector()  * 100.f;
		FHitResult WalkableSurfaceHitResult;
		GetWorld()->LineTraceSingleByProfile(WalkableSurfaceHitResult, WalkableSurfaceTraceStart, WalkableSurfaceTraceEnd, "BlockAll", TalesCharacterOwner->GetIgnoreCharacterParams());
		// FVector tmp = GetUnRotatedClimbVelocity();
		// SLOG(tmp.ToString());
		// SLOG(Velocity.ToString());
		if(WalkableSurfaceHitResult.bBlockingHit && GetUnRotatedClimbVelocity().Z > 15.f)
		{
			OnArriveTopStateDelegate.ExecuteIfBound();
			// @Todo delete
			if(OutClimbMontage)
			{
				SetMovementMode(MOVE_Flying);
				CharacterOwner->PlayAnimMontage(OutClimbMontage);
				MontageName = "ClimbOut";
			}
			StartNewPhysics(deltaTime, Iterations);
		}
	}
	
	RestorePreAdditiveRootMotionVelocity();

	// mapping Acceleration
	
	if(!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		CalcVelocity(deltaTime, 0.f, true, GetMaxBrakingDeceleration());
	}

	ApplyRootMotionToVelocity(deltaTime);
	
	FVector OldLocation = UpdatedComponent->GetComponentLocation();
	const FVector Adjusted = Velocity * deltaTime;
	const FQuat CurrentQuat = UpdatedComponent->GetComponentQuat();
	const FQuat TargetQuat = FRotationMatrix::MakeFromX(-CurrentClimbablefaceNormal).ToQuat();
	FQuat NewRotation = FMath::QInterpTo(CurrentQuat, TargetQuat, deltaTime, 5.f);
	FHitResult Hit(1.f);
	SafeMoveUpdatedComponent(Adjusted, NewRotation, true, Hit);
	if(Hit.Time < 1.f)
	{
		HandleImpact(Hit, deltaTime, Adjusted);
		SlideAlongSurface(Adjusted, (1.f - Hit.Time), Hit.Normal, Hit, true);
	}

	if(!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / deltaTime;
	}

	// Snap Movement to Climbable Surfaces
	const FVector ProjectedCharacterToSurface = (CurrentClimbableSurfaceLocation - UpdatedComponent->GetComponentLocation()).ProjectOnTo(UpdatedComponent->GetForwardVector());
	const FVector SnapVector = -CurrentClimbablefaceNormal * ProjectedCharacterToSurface.Length();
	UpdatedComponent->MoveComponent(SnapVector * deltaTime * GetMaxSpeed(), UpdatedComponent->GetComponentQuat(), true);
}

void UTalesCharacterMovementComponent::OnClimbMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if((Montage == TransitionClimbUpMontage || Montage == TransitionClimbDownMontage) && CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)
	{
		SLOG("Climb Montage ended");
		Safe_WantsToClimb = true;
		Safe_ClimbTransitionFinished = true;
	}
}

void UTalesCharacterMovementComponent::HandleHop(float SafeDistance,int Index)
{
	if(CheckCanHopUp(100.f, -30.f, SafeDistance) && ensureAlways(ClimbHopMontage[Index]))
	{
		// PlayClimbMontage(HopUpMontage);
		CharacterOwner->PlayAnimMontage(ClimbHopMontage[Index]);
	}
}

bool UTalesCharacterMovementComponent::CheckCanHopUp(float TraceDistance, float EyeOffset, float HopEndOffset)
{
	FHitResult HopUpHit;
	const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
	FVector EyeHeightOffset = UpdatedComponent->GetUpVector() * (CharacterOwner->BaseEyeHeight + EyeOffset);
	FVector Start = ComponentLocation + EyeHeightOffset;
	FVector End = Start + UpdatedComponent->GetForwardVector() * TraceDistance;
	GetWorld()->LineTraceSingleByProfile(HopUpHit, Start, End, "BlockAll", TalesCharacterOwner->GetIgnoreCharacterParams());
	FHitResult SaftyLedgeHit;	
	EyeHeightOffset = UpdatedComponent->GetUpVector() * (CharacterOwner->BaseEyeHeight + HopEndOffset);
	Start = ComponentLocation + EyeHeightOffset;
	End = Start + UpdatedComponent->GetForwardVector() * TraceDistance;
	GetWorld()->LineTraceSingleByProfile(SaftyLedgeHit, Start, End, "BlockAll", TalesCharacterOwner->GetIgnoreCharacterParams());
	
	if(HopUpHit.bBlockingHit && SaftyLedgeHit.bBlockingHit)
	{
		return true;
	}
	return false;
}

void UTalesCharacterMovementComponent::RequestHopping()
{
	const FVector UnrotatedLastInputVector = UKismetMathLibrary::Quat_UnrotateVector(UpdatedComponent->GetComponentQuat(), GetLastInputVector());

	const float DotResult = FVector::DotProduct(UnrotatedLastInputVector.GetSafeNormal(), FVector::UpVector);

	if(DotResult >= 0.9f)
	{
		HandleHop(150.f, 0);
	}else if(DotResult <= -0.9f)
	{
		HandleHop(-150.f, 1);
	}else if(DotResult >= 0.5 && DotResult <= 0.86)
	{
		if(FVector::DotProduct(UnrotatedLastInputVector.GetSafeNormal(), FVector::RightVector) > 0)
		{
			HandleHop(150.f, 2);
		}
		else
		{
			HandleHop(150.f, 3);
		}
	}
	
}
#pragma endregion
// ---------------------------------------       Trigger        ---------------------------------------------
#pragma region trigger
void UTalesCharacterMovementComponent::SprintPressed()
{
	Safe_WantToSprint = true;
}

void UTalesCharacterMovementComponent::SprintReleased()
{
	Safe_WantToSprint = false;
}

void UTalesCharacterMovementComponent::TalesInSlide()
{
	Safe_WantsToSlide = true;
}

void UTalesCharacterMovementComponent::TalesOutSlide()
{
	Safe_WantsToSlide = false;
}

void UTalesCharacterMovementComponent::TalesInProne()
{
}

void UTalesCharacterMovementComponent::TalesOutProne()
{
}
#pragma endregion
