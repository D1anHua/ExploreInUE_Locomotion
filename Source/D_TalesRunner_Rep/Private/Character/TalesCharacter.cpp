// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/TalesCharacter.h"

// #include "Components/AudioComponent.h"
#include "NiagaraComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Blueprint/UserWidget.h"
#include "Character/TalesCharacterMovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Inventory/TalesInventorInteractUI.h"
#include "Inventory/TalesInventoryComponent.h"
#include "Kismet/GameplayStatics.h"

// GAS
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/TalesAbilitySystemCompBase.h"
#include "AbilitySystem/TalesAttributeSetBase.h"
#include "DataAsset/CharacterDataAsset.h"
#include "AbilitySystemLog.h"

// Network
#include "Inventory/Data/InventoryItemInstance.h"
#include "Net/UnrealNetwork.h"

// TODO: Debug system include File, Delete later

// Sets default values
ATalesCharacter::ATalesCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UTalesCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

	TalesCharacterMovementComponent = Cast<UTalesCharacterMovementComponent>(GetCharacterMovement());
	TalesCharacterMovementComponent->SetIsReplicated(true);

	InventoryComponent = CreateDefaultSubobject<UTalesInventoryComponent>("TalesInventoryComponent");
	InventoryComponent->SetIsReplicated(true);
	
	// GAS
	AbilitySystemCompBase = CreateDefaultSubobject<UTalesAbilitySystemCompBase>(TEXT("AbilitySystemComponet"));
	AbilitySystemCompBase->SetIsReplicated(true);
	AbilitySystemCompBase->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>("SpringArmComp");
	SpringArmComp->SetupAttachment(RootComponent);

	CameraComp = CreateDefaultSubobject<UCameraComponent>("CameraComp");
	CameraComp->SetupAttachment(SpringArmComp);

	// Sprint Setting
	SprintTimeLineComp = CreateDefaultSubobject<UTimelineComponent>("SprintTimeLineComp");
	SprintLineNiagaraComp = CreateDefaultSubobject<UNiagaraComponent>("SprintLineVFXComp");
	SprintLineNiagaraComp->SetupAttachment(CameraComp);
	SprintLineNiagaraComp->SetRelativeRotation(FRotator(270.f, 0.f, 0.f));
	SprintLineNiagaraComp->SetRelativeScale3D(FVector(3.f, 3.f, 3.f));

	// MeshComponent
	SwardMesh = CreateDefaultSubobject<UStaticMeshComponent>("SwardMesh");
	SwardMesh->SetupAttachment(GetMesh(), TEXT("SwardSocket"));
	
	ShieldMesh = CreateDefaultSubobject<UStaticMeshComponent>("ShieldMesh");
	ShieldMesh->SetupAttachment(GetMesh(), TEXT("ShieldSocket"));


	AttributeSetBase = CreateDefaultSubobject<UTalesAttributeSetBase>(TEXT("AttributeSet"));
}

void ATalesCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	
	AddInputMappingContext(PCInputMapping, 0);
}

void ATalesCharacter::OnRep_ReplicatedAcceleration()
{
	if(TalesCharacterMovementComponent)
	{
		// Decompress Acceleration
		const double MaxAccel		   = TalesCharacterMovementComponent->MaxAcceleration;
		const double AcceleXYMagnitude = double(ReplicatedAcceleration.AccelXYMagnitude) * MaxAccel / 255.0;
		const double AcceleXYRadians   = double(ReplicatedAcceleration.AccelXYRadians) * TWO_PI / 255.0;
		
		FVector UnpackedAcceleration(FVector::ZeroVector);
		FMath::PolarToCartesian(AcceleXYMagnitude, AcceleXYRadians, UnpackedAcceleration.X, UnpackedAcceleration.Y);
		UnpackedAcceleration.Z = double(ReplicatedAcceleration.AccelZ) * MaxAccel / 127.0;

		TalesCharacterMovementComponent->SetReplicatedAcceleration(UnpackedAcceleration);
	}
}

// Called when the game starts or when spawned
void ATalesCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	SprintLineNiagaraComp->Deactivate();
	// Add Mappings for our game, more complex games may have multiple Contexts that are added/removed at runtime.
	// JetPackThrusterComp->Deactivate();
	// JetPackThrusterAudioComp->Activate();
	// JetPackThrusterAudioComp->Stop();
	// Interact Component
	if(GetLocalRole() == ROLE_AutonomousProxy)
	{
		InteractUI = CreateWidget<UTalesInventorInteractUI>(UGameplayStatics::GetPlayerController(GetWorld(),0), InteractUIClass);
	}
}

void ATalesCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if(IsValid(CharacterDataAsset))
	{
		SetCharacterData(CharacterDataAsset->CharacterData);	
	}
	
	UpdateSprintFOVTrack.BindDynamic(this, &ATalesCharacter::UpdateSprintFov);

	//If we have a float curve, bind it's graph to our update function
	if(SprintFovChangeFloatCurve)
	{
		SprintTimeLineComp->AddInterpFloat(SprintFovChangeFloatCurve, UpdateSprintFOVTrack);
	}

	if(TalesCharacterMovementComponent)
	{
		TalesCharacterMovementComponent->OnEnterClimbStateDelegate.BindUObject(this, &ThisClass::OnPlayerEnterClimbState);
		TalesCharacterMovementComponent->OnExitClimbStateDelegate.BindUObject(this, &ThisClass::OnPlayerExitClimbState);
		TalesCharacterMovementComponent->OnArriveTopStateDelegate.BindUObject(this, &ThisClass::OnReachTopClimbState);
	}
}

void ATalesCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATalesCharacter, CharacterData);
	DOREPLIFETIME(ATalesCharacter, InventoryComponent);
	DOREPLIFETIME_CONDITION(ATalesCharacter, ReplicatedAcceleration, COND_SimulatedOnly);
}

void ATalesCharacter::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);
	if(TalesCharacterMovementComponent)
	{
		// Compress Acceleration:  XY components as direction + magnitude, Z component as direct value
		const double MaxAccel = TalesCharacterMovementComponent->MaxAcceleration;
		const FVector CurrentAccell = TalesCharacterMovementComponent->GetCurrentAcceleration();
		double AccelXYRadians, AccelXYMagnitude;
		// 正交坐标 -> 笛卡尔坐标 换成方向和大小
		FMath::CartesianToPolar(CurrentAccell.X, CurrentAccell.Y, AccelXYMagnitude, AccelXYRadians);

		ReplicatedAcceleration.AccelXYRadians   = FMath::FloorToInt((AccelXYRadians / TWO_PI) * 255.0);
		ReplicatedAcceleration.AccelXYMagnitude = FMath::FloorToInt((AccelXYMagnitude / MaxAccel) * 255.0);
		ReplicatedAcceleration.AccelZ			= FMath::FloorToInt((CurrentAccell.Z / MaxAccel) * 127.0);
	}
}

void ATalesCharacter::AddInputMappingContext(UInputMappingContext* ContextToAdd, int32 InPriority)
{
	if(!ContextToAdd) return;

	if(APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if(UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(ContextToAdd, InPriority);
		}
	}
}

void ATalesCharacter::RemoveInputMappingContext(UInputMappingContext* ContextToAdd)
{
	if(!ContextToAdd) return;
	if(APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if(UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->RemoveMappingContext(ContextToAdd);
		}
	}
}

// Called to bind functionality to input
void ATalesCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);


	// New Enhanced Input System
	UEnhancedInputComponent* InputComp = Cast<UEnhancedInputComponent>(PlayerInputComponent);

	// General
	if(ensureAlways(Input_Move))	 InputComp->BindAction(Input_Move,   ETriggerEvent::Triggered, this, &ATalesCharacter::MoveFunc);
	if(ensureAlways(Input_ClimbMove)) InputComp->BindAction(Input_ClimbMove, ETriggerEvent::Triggered, this, &ATalesCharacter::ClimbMoveFunc);
	if(ensureAlways(Input_Jump))	 InputComp->BindAction(Input_Jump,   ETriggerEvent::Triggered, this, &ATalesCharacter::OnJumpActionStart);
	if(ensureAlways(Input_ClimbHop)) InputComp->BindAction(Input_ClimbHop, ETriggerEvent::Triggered, this, &ATalesCharacter::OnClimbHopActionStart);
	if(ensureAlways(Input_LookMouse))InputComp->BindAction(Input_LookMouse, ETriggerEvent::Triggered, this, &ATalesCharacter::LookMouse);
	if(ensureAlways(Input_Crouch))   InputComp->BindAction(Input_Crouch, ETriggerEvent::Started, this, &ATalesCharacter::OnCrouchActionStart);
	if(ensureAlways(Input_Crouch))   InputComp->BindAction(Input_Crouch, ETriggerEvent::Completed, this, &ATalesCharacter::OnCrouchActionEnd);
	if(ensureAlways(Input_Dash))	 InputComp->BindAction(Input_Dash, ETriggerEvent::Triggered, this, &ATalesCharacter::OnDashActionStart);
	if(ensureAlways(Input_Slide))	 InputComp->BindAction(Input_Slide, ETriggerEvent::Triggered, this, &ATalesCharacter::OnSlideStart);
	if(ensureAlways(Input_Prone))	 InputComp->BindAction(Input_Prone, ETriggerEvent::Triggered, this, &ATalesCharacter::OnProneStart);
	
	// Sprint while key is held
	if(ensureAlways(Input_Sprint))	 InputComp->BindAction(Input_Sprint, ETriggerEvent::Started,   this, &ATalesCharacter::OnSprintActionStart);
	if(ensureAlways(Input_Sprint))	 InputComp->BindAction(Input_Sprint, ETriggerEvent::Completed, this, &ATalesCharacter::OnSprintActionEnd);
	
	if(ensureAlways(Input_Climb))	 InputComp->BindAction(Input_Climb, ETriggerEvent::Triggered,   this, &ATalesCharacter::OnClimbActionStart);
	
	if(ensureAlways(Input_Equip))	 InputComp->BindAction(Input_Equip,  ETriggerEvent::Triggered,   this, &ATalesCharacter::OnEquipItemTriggered);
	if(ensureAlways(Input_Drop))	 InputComp->BindAction(Input_Drop  , ETriggerEvent::Triggered, this, &ATalesCharacter::OnDropItemTriggered);
}



void ATalesCharacter::OnPlayerEnterClimbState()
{
	ensureAlways(ClimbInputMapping);
	AddInputMappingContext(ClimbInputMapping, 1);	
}

void ATalesCharacter::OnPlayerExitClimbState()
{
	RemoveInputMappingContext(ClimbInputMapping);
	if(AbilitySystemCompBase)
	{
		AbilitySystemCompBase->CancelAbilities(&ClimbTags);
	}
}

void ATalesCharacter::OnReachTopClimbState()
{
	// 播放Montage
	if(AbilitySystemCompBase)
	{
		AbilitySystemCompBase->TryActivateAbilitiesByTag(ClimbReachTags, true);
	}	
}

void ATalesCharacter::MoveFunc(const FInputActionInstance& Instance)
{
	FRotator ControlRot = GetControlRotation();

	// Get Value from input (Combined value from up down right left keys) and convert to Vector2D
	const FVector2d AxisValue = Instance.GetValue().Get<FVector2d>();

	// Move forward / back
	const FVector ForVector = FRotationMatrix(ControlRot).GetUnitAxis(EAxis::X);
	AddMovementInput(ControlRot.Vector(), AxisValue.X);

	// Move Right / Left
	const FVector RightVector = FRotationMatrix(ControlRot).GetUnitAxis(EAxis::Y);
	AddMovementInput(RightVector, AxisValue.Y);
}

void ATalesCharacter::ClimbMoveFunc(const FInputActionInstance& Instance)
{
	FVector2d MovementVector = Instance.GetValue().Get<FVector2d>();

	const FVector ForwardDirection = FVector::CrossProduct(-TalesCharacterMovementComponent->GetClimbableSurfaceNormal(), GetActorRightVector());

	const FVector RightDirection = FVector::CrossProduct(-TalesCharacterMovementComponent->GetClimbableSurfaceNormal(), -GetActorUpVector());

	AddMovementInput(ForwardDirection, MovementVector.X);
	AddMovementInput(RightDirection, MovementVector.Y);
}

void ATalesCharacter::SprintStart(const FInputActionInstance& Instance)
{
	// TODO: Can follow the method in the RPG tutorial to set it up later.
	// Now just complete in this function
	TalesCharacterMovementComponent->SprintPressed();
	if(GetVelocity().Length() != 0.f)
	{
		bIsSprint = true;
		// TimeLine to Control Fov
		SprintTimeLineComp->Play();
		SprintLineNiagaraComp->Activate();
		if(ensureAlways(SprintShake))
		{
			CurrentSprintShake =  GetWorld()->GetFirstPlayerController()->PlayerCameraManager->StartCameraShake(SprintShake, 1);
		}
		// TODO: Add Camera Shake later, But not use today.

	}	
}

void ATalesCharacter::SprintStop(const FInputActionInstance& Instance)
{
	TalesCharacterMovementComponent->SprintReleased();
	if(bIsSprint)
	{
		bIsSprint = false;
		SprintTimeLineComp->Reverse();
		SprintLineNiagaraComp->Deactivate();
		if(CurrentSprintShake)
		{
			GetWorld()->GetFirstPlayerController()->PlayerCameraManager->StopCameraShake(CurrentSprintShake, true);
			CurrentSprintShake = nullptr;
		}
	}
}

void ATalesCharacter::LookMouse(const FInputActionInstance& Instance)
{
	const FVector2D Value = Instance.GetValue().Get<FVector2D>();
	// @todo 换个位置实现?
	CameraTurnRate = FMath::Clamp(Value.X, -1.f, 1.f);
	AddControllerYawInput(Value.X);
	AddControllerPitchInput(Value.Y);
}

void ATalesCharacter::ClimbHop(const FInputActionInstance& Instance)
{
	if(TalesCharacterMovementComponent)
	{
		TalesCharacterMovementComponent->RequestHopping();
	}
}

void ATalesCharacter::OnSlideStart(const FInputActionInstance& Instance)
{
	if(TalesCharacterMovementComponent)
	{
		TalesCharacterMovementComponent->TalesInSlide();
	}
}

void ATalesCharacter::OnProneStart(const FInputActionInstance& Instance)
{
	if(TalesCharacterMovementComponent)
	{
		if(TalesCharacterMovementComponent->IsProne())
		{
			TalesCharacterMovementComponent->TalesOutProne();
		}
		else
		{
			TalesCharacterMovementComponent->TalesInProne();
		}
	}
}

void ATalesCharacter::OnJumpActionStart(const FInputActionInstance& Instance)
{
	FGameplayEventData Payload;

	Payload.Instigator = this;
	Payload.EventTag = JumpEventTag;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, JumpEventTag, Payload);
}

void ATalesCharacter::OnCrouchActionStart(const FInputActionInstance& Instance)
{
	if(AbilitySystemCompBase)
	{
		AbilitySystemCompBase->TryActivateAbilitiesByTag(CrouchTags, true);
	}
}

void ATalesCharacter::OnCrouchActionEnd(const FInputActionInstance& Instance)
{
	if(AbilitySystemCompBase)
	{
		AbilitySystemCompBase->CancelAbilities(&CrouchTags);
	}
}

void ATalesCharacter::OnSprintActionStart(const FInputActionInstance& Instance)
{
	if(AbilitySystemCompBase)
	{
		AbilitySystemCompBase->TryActivateAbilitiesByTag(SprintTags, true);
	}
}

void ATalesCharacter::OnSprintActionEnd(const FInputActionInstance& Instance)
{
	if(AbilitySystemCompBase)
	{
		AbilitySystemCompBase->CancelAbilities(&SprintTags);
	}
}

void ATalesCharacter::OnDashActionStart(const FInputActionInstance& Instance)
{
	if(AbilitySystemCompBase)
	{
		AbilitySystemCompBase->TryActivateAbilitiesByTag(DashTags, true);
	}	
}

void ATalesCharacter::OnClimbActionStart(const FInputActionInstance& Instance)
{
	if(AbilitySystemCompBase)
	{
		if(TalesCharacterMovementComponent->IsFlying() || TalesCharacterMovementComponent->IsClimbing())
		{
			AbilitySystemCompBase->CancelAbilities(&ClimbTags);
		}
		else
		{
			AbilitySystemCompBase->TryActivateAbilitiesByTag(ClimbTags, true);
		}
	}
}

void ATalesCharacter::OnClimbActionEnd(const FInputActionInstance& Instance){
}

void ATalesCharacter::OnClimbHopActionStart(const FInputActionInstance& Instance)
{
	if(AbilitySystemCompBase)
	{
		AbilitySystemCompBase->TryActivateAbilitiesByTag(ClimbHopTags, true);
	}	
}

void ATalesCharacter::OnDropItemTriggered(const FInputActionInstance& Instance)
{
	FGameplayEventData EventPayload;
	EventPayload.EventTag = UTalesInventoryComponent::DropItemTag;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, UTalesInventoryComponent::DropItemTag, EventPayload);
}

void ATalesCharacter::OnEquipItemTriggered(const FInputActionInstance& Instance)
{
	FGameplayEventData EventPayload;
	EventPayload.EventTag = UTalesInventoryComponent::EquipTag;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, UTalesInventoryComponent::EquipTag, EventPayload);
}

void ATalesCharacter::Jump()
{
	bPressedTalesJump = true;
	Super::Jump();
	bPressedJump = false;
	// UE_LOG(LogTemp, Warning, TEXT("Jump is Server: %d"), HasAuthority());
}

void ATalesCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	if(TalesCharacterMovementComponent)
	{
		auto VelocityZ = GetVelocity().Z;
		if(VelocityZ >= 300.0 && VelocityZ < 900.0)
		{
			LandState = Normal;
		}else if(VelocityZ >= 900.0)
		{
			LandState = Soft;
		}else
		{
			LandState = Heavy;
		}
	}	
	if(AbilitySystemCompBase)
	{
		AbilitySystemCompBase->RemoveActiveEffectsWithTags(InAirTags);
	}
}

void ATalesCharacter::StopJumping()
{
	bPressedTalesJump = false;
	Super::StopJumping();
}

void ATalesCharacter::UpdateSprintFov(float TimelineOutput)
{
	float CurrentFov = TimelineOutput * (SprintFov - DefaultFov) + DefaultFov;
	CameraComp->SetFieldOfView(CurrentFov);
}

void ATalesCharacter::ActivateInteractUI()
{
	if(!InteractUI)
	{
		InteractUI = CreateWidget<UTalesInventorInteractUI>(GetWorld(), InteractUIClass);	
	}
	
	if(!InteractUI->IsInViewport())
	{
		InteractUI->AddToViewport();
	}
}

void ATalesCharacter::UnActivateInteractUI()
{
	if(InteractUI->IsInViewport())
	{
		InteractUI->RemoveFromParent();
	}
}

void ATalesCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	if(!CrouchStateEffect.Get()) return;
	if(AbilitySystemCompBase)
	{
		FGameplayEffectContextHandle EffectContext = AbilitySystemCompBase->MakeEffectContext();
		FGameplayEffectSpecHandle SpecHandle = AbilitySystemCompBase->MakeOutgoingSpec(CrouchStateEffect.Get(), 1, EffectContext);
		if(SpecHandle.IsValid())
		{
			FActiveGameplayEffectHandle ActiveGEHandle = AbilitySystemCompBase->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			if(!ActiveGEHandle.WasSuccessfullyApplied())
			{
				ABILITY_LOG(Log, TEXT("Ability %s failed to apply crouch effect %s"), *GetNameSafe(CrouchStateEffect));
			}
		}
	}
}

void ATalesCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	if(AbilitySystemCompBase && CrouchStateEffect.Get())
	{
		AbilitySystemCompBase->RemoveActiveGameplayEffectBySourceEffect(CrouchStateEffect, AbilitySystemCompBase);
	}
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
}

FCollisionQueryParams ATalesCharacter::GetIgnoreCharacterParams() const
{
	FCollisionQueryParams Params;

	TArray<AActor*> CharacterChileren;
	GetAllChildActors(CharacterChileren);
	Params.AddIgnoredActors(CharacterChileren);
	Params.AddIgnoredActor(this);

	return Params;
}

void ATalesCharacter::SetSwardMesh(UStaticMesh* InSwardMesh)
{
	this->SwardMesh->SetStaticMesh(InSwardMesh);
}

void ATalesCharacter::SetShieldMesh(UStaticMesh* InShieldMesh)
{
	this->ShieldMesh->SetStaticMesh(InShieldMesh);
}

#pragma region GAS
bool ATalesCharacter::ApplyGameplayEffectToSelf(TSubclassOf<UGameplayEffect> Effect, FGameplayEffectContextHandle InEffectContext)
{
	if(!Effect.Get()) return false;

	FGameplayEffectSpecHandle SpecHandle = AbilitySystemCompBase->MakeOutgoingSpec(Effect, 1, InEffectContext);
	if(SpecHandle.IsValid())
	{
		FActiveGameplayEffectHandle ActiveGEHandle = AbilitySystemCompBase->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		return ActiveGEHandle.WasSuccessfullyApplied();
	}
	return false;
}

void ATalesCharacter::GiveAbilities()
{
	if(HasAuthority() && AbilitySystemCompBase)
	{
		for(auto DefaultAbility : CharacterData.Abilities)
		{
			AbilitySystemCompBase->GiveAbility(FGameplayAbilitySpec(DefaultAbility));
		}
	}
}

void ATalesCharacter::ApplyStartupEffects()
{
	if(GetLocalRole() == ROLE_Authority)
	{
		FGameplayEffectContextHandle EffectContext = AbilitySystemCompBase->MakeEffectContext();
		EffectContext.AddSourceObject(this);

		for(auto& CharacterEffect : CharacterData.Effects)
		{
			ApplyGameplayEffectToSelf(CharacterEffect, EffectContext);
		}
	}
}

void ATalesCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	AbilitySystemCompBase->InitAbilityActorInfo(this, this);
	
	GiveAbilities();
	ApplyStartupEffects();
}

void ATalesCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	AbilitySystemCompBase->InitAbilityActorInfo(this, this);
}

UAbilitySystemComponent* ATalesCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemCompBase;	
}

void ATalesCharacter::OnRep_CharacterData()
{
	InitFromCharacterData(CharacterData, true);
}

void ATalesCharacter::InitFromCharacterData(const FCharacterData& InCharacterData, bool bFromReplication)
{
	
}
#pragma endregion

// Called every frame
void ATalesCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

