// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/TalesCharacterAnimInstance.h"

#include "KismetAnimationLibrary.h"
#include "Character/TalesCharacter.h"
#include "Character/TalesCharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UTalesCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	TalesCharacter = Cast<ATalesCharacter>(TryGetPawnOwner());
	if(TalesCharacter)
	{
		TalesCharacterMovementComponent = TalesCharacter->GetTalesCharacterMovement();
	}
}

void UTalesCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
}

void UTalesCharacterAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);
	if(!TalesCharacter || !TalesCharacterMovementComponent) return;
	UpdateLocationData(DeltaSeconds);
	UpdateRotationData();
	UpdateVelocityData();
	UpdateAccelerationData();

	// 用速度来计算
	GetJumpParams();
	GetCustomMode();
	UpdateWallDetection();

	bIsFirstUpdate = false;
}

void UTalesCharacterAnimInstance::UpdateLocationData(float DeltaSeconds)
{
	const auto NowLocation = TalesCharacterMovementComponent->GetActorLocation();
	if(bIsFirstUpdate)
	{
		DisplacementSpeed = 0.f;
		DisplacementSinceLastUpdate = 0.f;
	}else
	{
		
		auto LocationGaps = (NowLocation - WorldLocation);
		LocationGaps.Z = 0;
		DisplacementSinceLastUpdate = LocationGaps.Length();
		DisplacementSpeed = DisplacementSinceLastUpdate / DeltaSeconds;	
	}
	WorldLocation = NowLocation;

	RootPitch = (TalesCharacter->GetBaseAimRotation() - TalesCharacter->GetActorRotation()).Pitch;
	RotationAngle = FMath::FInterpTo(RotationAngle, TalesCharacter->CameraTurnRate, DeltaSeconds,10.f);
}

void UTalesCharacterAnimInstance::UpdateRotationData()
{
	if(bIsFirstUpdate)
	{
		YawDeltaSinceLastUpdate = 0.f;
		// AdditiveLeanAngle = 0.f;
	}
	const auto OldRotation = WorldRotation;
	LastWorldRotation = WorldRotation;
	WorldRotation  = TalesCharacter->GetActorRotation();
	YawDeltaSinceLastUpdate =  OldRotation.Yaw - WorldRotation.Yaw;
	YawDeltaSpeed = YawDeltaSinceLastUpdate / GetDeltaSeconds();
}

void UTalesCharacterAnimInstance::UpdateVelocityData()
{
	bool bIsMovingLastUpdate = !FVector2d(Velocity.X, Velocity.Y).IsNearlyZero();
	GetGroundSpeed();
	GetAirSpeed();
	GetVelocityComponent();
	GetShouldMove();
	const FVector WorldVelocity2D(Velocity.X, Velocity.Y, 0);
	LocalVelocity2D = UKismetMathLibrary::Quat_UnrotateVector(WorldRotation.Quaternion(), WorldVelocity2D);
	bHasVelocity = LocalVelocity2D.IsNearlyZero();
	LocalVelocityDirectionAngle = UKismetAnimationLibrary::CalculateDirection(WorldVelocity2D, WorldRotation);
	LocalVelocityDirectionAngleWithOffset = LocalVelocityDirectionAngle - RootYawOffset;
	LocalVelocityDirection = SelectCardinalDirectionfromAngle(LocalVelocityDirectionAngleWithOffset, CardinalDirectionDeadZone, LocalVelocityDirection, bIsMovingLastUpdate);
	LocalVelocityDirectionNoOffset = SelectCardinalDirectionfromAngle(LocalVelocityDirectionAngle, CardinalDirectionDeadZone, LocalVelocityDirectionNoOffset, bIsMovingLastUpdate);
}

void UTalesCharacterAnimInstance::UpdateAccelerationData()
{
	WorldAcceleration2D = TalesCharacterMovementComponent->GetCurrentAcceleration();
	WorldAcceleration2D.Z = 0;
	LocalAcceleration2D = UKismetMathLibrary::Quat_UnrotateVector(WorldRotation.Quaternion(), WorldAcceleration2D);
	bHasAcceleration = LocalAcceleration2D.IsNearlyZero();

	UpdateMovementDirection();
	// Calculate a cardinal direction to be used for pivots.
	PivotDirection2D = FMath::Lerp(PivotDirection2D, WorldAcceleration2D.GetSafeNormal(), 0.5f).GetSafeNormal();
	float angle = UKismetAnimationLibrary::CalculateDirection(PivotDirection2D, WorldRotation);
	auto CurrentDirection = SelectCardinalDirectionfromAngle(angle, CardinalDirectionDeadZone, ForwardDirection, false);
	switch (CurrentDirection)
	{
	case ForwardDirection:
		CardinalDirectionFromAcceleration = BackwardDirection;
		break;
	case BackwardDirection:
		CardinalDirectionFromAcceleration = ForwardDirection;
		break;
	case LeftDirection:
		CardinalDirectionFromAcceleration = RightDirection;
	case RightDirection:
		CardinalDirectionFromAcceleration = LeftDirection;
	}

	bIsMovingPerpendicularToInitialPivot = (PivotInitialDirection == ForwardDirection || PivotInitialDirection == BackwardDirection) &&
		                                   (LocalVelocityDirection != ForwardDirection || LocalVelocityDirection != BackwardDirection);
	bIsMovingPerpendicularToInitialPivot |= (PivotInitialDirection == LeftDirection || PivotInitialDirection == RightDirection) &&
										    (LocalVelocityDirection != LeftDirection || LocalVelocityDirection != RightDirection);
	return;
}

void UTalesCharacterAnimInstance::GetGroundSpeed()
{
	Velocity = TalesCharacter->GetVelocity();
	GroundSpeed = Velocity.Length();
}

void UTalesCharacterAnimInstance::GetAirSpeed()
{
	AirSpeed = TalesCharacter->GetVelocity().Z;
}

void UTalesCharacterAnimInstance::GetVelocityComponent()
{
	FVector TempV = TalesCharacterMovementComponent->GetUnRotatedClimbVelocity();
	VerticalVelocity = TalesCharacterMovementComponent->GetUnRotatedClimbVelocity().Z;
	HorizontalVelocity = TalesCharacterMovementComponent->GetUnRotatedClimbVelocity().Y;
}

void UTalesCharacterAnimInstance::GetShouldMove()
{
	bShouldMove = TalesCharacterMovementComponent->GetCurrentAcceleration().Size() > 0 && GroundSpeed > UE_KINDA_SMALL_NUMBER && !TalesCharacterMovementComponent->IsFalling();
}

void UTalesCharacterAnimInstance::GetJumpParams()
{
	bIsOnGround = TalesCharacterMovementComponent->IsMovingOnGround();
	bIsFalling = TalesCharacterMovementComponent->IsFalling();
	bIsJumping = false; bIsFallingJump = false;
	if(bIsFalling && AirSpeed > 0.f)
	{
		bIsJumping = true;
		TimeJumpToApex = (0.f - AirSpeed) / TalesCharacterMovementComponent->GetGravityZ();
	}
	else
	{
		bIsFallingJump = true;
		TimeJumpToApex = 0.f;

		// Trace to Ground Distance
		FHitResult HitResult;
		FVector End = TalesCharacter->GetActorLocation();
		End.Z -= (300.f + TalesCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		GetWorld()->LineTraceSingleByChannel(HitResult, TalesCharacter->GetActorLocation(), End, ECC_Visibility, TalesCharacter->GetIgnoreCharacterParams());
		if(HitResult.bBlockingHit)
		{
			GroundDistance = HitResult.Distance;	
		}
	}
	LandState	 = TalesCharacter->LandState;
}

void UTalesCharacterAnimInstance::GetCustomMode()
{
	bIsSliding   = TalesCharacterMovementComponent->IsSlide();
	bIsCrouching = TalesCharacterMovementComponent->IsCrouching();	
	bIsProning   = TalesCharacterMovementComponent->IsProne();	
	bIsClimbing  = TalesCharacterMovementComponent->IsClimbing();
	bIsSprint    = TalesCharacterMovementComponent->IsSprint();
}

void UTalesCharacterAnimInstance::UpdateWallDetection()
{
	const auto Accelerate = TalesCharacterMovementComponent->GetCurrentAcceleration();
	const FVector2d Acceleration2D(Accelerate.X, Accelerate.Y);
	const FVector2d Velocity2D(Velocity.X,Velocity.Y);
	const auto Angle = Acceleration2D.GetSafeNormal().Dot(Velocity2D.GetSafeNormal());
	bIsIntoWall = Acceleration2D.Length() > 0.2 && Velocity2D.Length() > 200.f && (Angle > -0.6 && Angle < 0.6);
}

void UTalesCharacterAnimInstance::UpdateMovementDirection()
{
	MovementDirection = UKismetAnimationLibrary::CalculateDirection(Velocity, WorldRotation);
	MovementDirection = UKismetMathLibrary::NormalizeAxis(MovementDirection);
	if(MovementDirection >= -70.f && MovementDirection <= 70.f)
	{
		MovementDirectionEnum = ForwardDirection;
	}
	else if(MovementDirection >= 70.f && MovementDirection <= 110.f)
	{
		MovementDirectionEnum = RightDirection;
	}
	else if(MovementDirection >= -110.f && MovementDirection <= -70.f)
	{
		MovementDirectionEnum = LeftDirection;
	}
	else
	{
		MovementDirectionEnum = BackwardDirection;
	}
	UpdateOrientationAngle();
}

void UTalesCharacterAnimInstance::UpdateOrientationAngle()
{
	FOrientationAngle = MovementDirection - 0.f;
	ROrientationAngle = MovementDirection - 90.f;
	BOrientationAngle = MovementDirection - 180.f;
	LOrientationAngle = MovementDirection + 90.f;
}

EAnimEnumCardinalDirection UTalesCharacterAnimInstance::SelectCardinalDirectionfromAngle(float angle, float DeadZone,
                                                                                         EAnimEnumCardinalDirection CurrentDirection, bool bUseCurrentDirection)
{
	float AbsAngle = FMath::Abs(angle);
	float FwdDeadZone = DeadZone;
	float BwdDeadZone = DeadZone;
	if(bUseCurrentDirection)
	{
		switch (CurrentDirection)
		{
		case ForwardDirection:
			FwdDeadZone *= 2.f;
			break;
		case BackwardDirection:
			BwdDeadZone *= 2.f;
			break;
		default:
			break;
		}
	}

	if(AbsAngle <= (FwdDeadZone + 45.f))
	{
		return ForwardDirection;
	}
	else
	{
		if(AbsAngle >= (135.f - BwdDeadZone))
		{
			return BackwardDirection;
		}
		else if(angle > 0.f)
		{
			return RightDirection;
		}
		else
		{
			return LeftDirection;
		}
	}
	return ForwardDirection;
}
