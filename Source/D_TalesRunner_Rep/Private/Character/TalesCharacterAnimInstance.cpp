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
	if(!TalesCharacter || !TalesCharacterMovementComponent) return;
	UpdateLocationData(DeltaSeconds);
	UpdateRotationData();
	UpdateVelocityData();
	UpdateAccelerationData();

	// 用速度来计算
	GetJumpParams();
	GetCustomMode();
	UpdateWallDetection();
	UpdateRootYawOffset();

	bIsFirstUpdate = false;
}

void UTalesCharacterAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);
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
		AdditiveLeanAngle = 0.f;
	}
	const auto OldRotation = WorldRotation;
	LastWorldRotation = WorldRotation;
	WorldRotation  = TalesCharacter->GetActorRotation();
	YawDeltaSinceLastUpdate =  WorldRotation.Yaw - OldRotation.Yaw;
	YawDeltaSpeed = YawDeltaSinceLastUpdate / GetDeltaSeconds();
	AdditiveLeanAngle = ((bIsCrouching) ?  0.025 : 0.0375) * YawDeltaSpeed;
	const FRotator ControlRotator = TalesCharacter->GetControlRotation();
	Pitch = (ControlRotator + FRotator(0.f, 180.f, 0.f)).Pitch;
	PitchRotator = {Pitch, 0.f, 0.f};
}

void UTalesCharacterAnimInstance::UpdateVelocityData()
{
	const bool bIsMovingLastUpdate = !LocalVelocity2D.IsNearlyZero();
	
	GetGroundSpeed();
	GetAirSpeed();
	GetVelocityComponent();
	GetShouldMove();
	const FVector WorldVelocity2D(Velocity.X, Velocity.Y, 0);
	LocalVelocity2D = UKismetMathLibrary::Quat_UnrotateVector(WorldRotation.Quaternion(), WorldVelocity2D);
	bHasVelocity = !LocalVelocity2D.IsNearlyZero();
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
	bHasAcceleration = !LocalAcceleration2D.IsNearlyZero();

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
		break;
	case RightDirection:
		CardinalDirectionFromAcceleration = LeftDirection;
		break;
	default:
		break;
	}

	bIsMovingPerpendicularToInitialPivot = (PivotInitialDirection == ForwardDirection || PivotInitialDirection == BackwardDirection) &&
		                                   (LocalVelocityDirection == LeftDirection || LocalVelocityDirection == RightDirection);
	bIsMovingPerpendicularToInitialPivot |= (PivotInitialDirection == LeftDirection || PivotInitialDirection == RightDirection) &&
										    (LocalVelocityDirection == BackwardDirection || LocalVelocityDirection == ForwardDirection);
	return;
}

void UTalesCharacterAnimInstance::UpdateRootYawOffset()
{
	//当脚不动时（例如空闲时），将根部偏移到与Pawn Owner旋转方向相反的方向，以防止网格随Pawn旋转。
	if(RootYawOffsetMode == Accumulate)
	{
		SetRootYawOffset(RootYawOffset - YawDeltaSinceLastUpdate);	
	}
	if(RootYawOffsetMode == BlendOut)
	{
		// 当运动是, 平滑的过度到对应的offset
		SetRootYawOffset(UKismetMathLibrary::FloatSpringInterp(RootYawOffset, 0.f, RootYawOffsetSpringState, 80.f, 1.0, GetDeltaSeconds(), 1.0, 0.5));
	}
	RootYawOffsetMode = BlendOut;

	AimPitch =  UKismetMathLibrary::NormalizeAxis(TalesCharacter->GetBaseAimRotation().Pitch);
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
	auto OldCrouch = bIsCrouching;
	bIsCrouching = TalesCharacterMovementComponent->IsCrouching();
	bCrouchStateChange = bIsCrouching ^ OldCrouch;
	bIsProning   = TalesCharacterMovementComponent->IsProne();	
	bIsClimbing  = TalesCharacterMovementComponent->IsClimbing();
	bIsSprint    = TalesCharacterMovementComponent->IsSprint();
}

void UTalesCharacterAnimInstance::SetRootYawOffset(float InRootYawOffset)
{
	//TurnInPlace #3: 简单来说就是为了避免过快的controller转动, 这里对rootYawOffset进行限制
	// We clamp the offset because at large offsets the character has to aim too far backwards, which over twists the spine. The turn in place animations will usually keep up with the offset, but this clamp will cause the feet to slide if the user rotates the camera too quickly.
	// If desired, this clamp could be replaced by having aim animations that can go up to 180 degrees or by triggering turn in place animations more aggressively.
	const FVector2D Clamp = bIsCrouching ? RootYawOffsetAngleClamp : RootYawOffsetAngleClampCrouched;
	RootYawOffset = UKismetMathLibrary::ClampAngle(InRootYawOffset, Clamp.X, Clamp.Y);
	AimYaw = -1 * RootYawOffset;
}

void UTalesCharacterAnimInstance::UpdateWallDetection()
{
	const auto Accelerate = TalesCharacterMovementComponent->GetCurrentAcceleration();
	const FVector2d Acceleration2D(Accelerate.X, Accelerate.Y);
	const FVector2d Velocity2D(Velocity.X,Velocity.Y);
	const auto Angle = Acceleration2D.GetSafeNormal().Dot(Velocity2D.GetSafeNormal());
	bIsIntoWall = Acceleration2D.Length() > 0.2 && Velocity2D.Length() < 200.f && (Angle > -0.6 && Angle < 0.6);
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
