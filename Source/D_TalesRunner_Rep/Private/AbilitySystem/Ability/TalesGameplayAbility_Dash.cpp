// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Ability/TalesGameplayAbility_Dash.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/TalesCharacter.h"
#include "Kismet/KismetMathLibrary.h"

// Cheat Debug Config
// TODO: 可以声明在GameInstance中
static TAutoConsoleVariable<int32> CVarDebugAbility(
	TEXT("ShowDebugAbility"),
	0,
	TEXT("Draws debug info about Ability")
	TEXT("0: Off/n")
	TEXT("1: On/n"),
	ECVF_Cheat
);

UTalesGameplayAbility_Dash::UTalesGameplayAbility_Dash()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UTalesGameplayAbility_Dash::CommitCheck(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, FGameplayTagContainer* OptionalRelevantTags)
{
	if(!Super::CommitCheck(Handle, ActorInfo, ActivationInfo, OptionalRelevantTags))
	{
		return false;
	}

	ATalesCharacter* Character = GetTalesCharacterFromActionInfo();
	if(Character == nullptr)
	{
		return false;
	}

	const auto MovementComponent = Character->GetCharacterMovement();
	// 这部分也许也不用这样, 直接GameplayTag之间的限制就行
	if(!(MovementComponent->IsWalking() && !MovementComponent->IsCrouching() || MovementComponent->IsFalling()))
	{
		return false;	
	}
	RequestDashDirection();
	if(DashMontage == nullptr)
	{
		// Default Montage
		DashMontage =AnimMontages.DDirectionMontage;
	}
	return true;
}

void UTalesGameplayAbility_Dash::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if(!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		K2_EndAbility();
		return;
	}
	const ATalesCharacter* Character = GetTalesCharacterFromActionInfo();
	UCharacterMovementComponent* MovementComponent = Character ? Character->GetCharacterMovement() : nullptr;
	if(MovementComponent)
	{
		MovementComponent->SetMovementMode(MOVE_Flying);
	}
	// 不考虑作弊问题
	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, DashMontage);

	MontageTask->OnBlendOut.AddDynamic(this, &UTalesGameplayAbility_Dash::K2_EndAbility);
	MontageTask->OnCancelled.AddDynamic(this, &UTalesGameplayAbility_Dash::K2_EndAbility);
	MontageTask->OnInterrupted.AddDynamic(this, &UTalesGameplayAbility_Dash::K2_EndAbility);
	MontageTask->OnCompleted.AddDynamic(this, &UTalesGameplayAbility_Dash::K2_EndAbility);
	MontageTask->ReadyForActivation();
}

void UTalesGameplayAbility_Dash::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if(IsValid(MontageTask))
	{
		MontageTask->EndTask();	
	}
	const ATalesCharacter* Character = GetTalesCharacterFromActionInfo();
	UCharacterMovementComponent* MovementComponent = Character ? Character->GetCharacterMovement() : nullptr;
	if(MovementComponent)
	{
		MovementComponent->SetMovementMode(MOVE_Falling);
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UTalesGameplayAbility_Dash::RequestDashDirection()
{
	const ATalesCharacter* Character = GetTalesCharacterFromActionInfo();
	UCharacterMovementComponent* MovementComponent = Character ? Character->GetCharacterMovement() : nullptr;
	if(MovementComponent)
	{
		const FVector UnrotatedLastInputVector = UKismetMathLibrary::Quat_UnrotateVector(Character->GetActorRotation().Quaternion(), MovementComponent->GetLastInputVector());

		const float DotResult = FVector::DotProduct(UnrotatedLastInputVector.GetSafeNormal(), FVector(1.f, 0.f, 0.f));

		if(DotResult >= 0.9f)
		{
			DashMontage = AnimMontages.FDirectionMontage;
		}
		else if(DotResult <= -0.9f)
		{
			DashMontage = AnimMontages.DDirectionMontage;
		}
		else
		{
			const auto Left = FVector::DotProduct(UnrotatedLastInputVector.GetSafeNormal(), FVector(0.f, 1.f,0.f));
			if(Left > 0.8f)
			{
				DashMontage = AnimMontages.RDirectionMontage;
			}
			else if(Left < -0.8f)
			{
				DashMontage = AnimMontages.LDirectionMontage;
			}
		}
		
	}
}
