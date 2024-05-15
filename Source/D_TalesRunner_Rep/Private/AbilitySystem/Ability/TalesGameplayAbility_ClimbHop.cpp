// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Ability/TalesGameplayAbility_ClimbHop.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/TalesCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"

UTalesGameplayAbility_ClimbHop::UTalesGameplayAbility_ClimbHop()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UTalesGameplayAbility_ClimbHop::CommitCheck(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, FGameplayTagContainer* OptionalRelevantTags)
{
	if(!Super::CommitCheck(Handle, ActorInfo, ActivationInfo, OptionalRelevantTags))
	{
		return false;
	}
	const ATalesCharacter* Character = GetTalesCharacterFromActionInfo();
	UTalesCharacterMovementComponent* MovementComponent = Character ? Character->GetTalesCharacterMovement() : nullptr;
	if(MovementComponent == nullptr ||  !MovementComponent->IsClimbing())
	{
		return false;
	}
	if(!RequestHopDirectionAndTest())
	{
		return false;	
	}
	return HopMontage != nullptr;
}

void UTalesGameplayAbility_ClimbHop::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if(!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		K2_EndAbility();
		return;
	}

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, HopMontage);

	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::K2_EndAbility);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::K2_EndAbility);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::K2_EndAbility);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::K2_EndAbility);
	MontageTask->ReadyForActivation();
}

void UTalesGameplayAbility_ClimbHop::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if(IsValid(MontageTask))
	{
		MontageTask->EndTask();
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UTalesGameplayAbility_ClimbHop::RequestHopDirectionAndTest()
{
	const ATalesCharacter* Character = GetTalesCharacterFromActionInfo();
	UTalesCharacterMovementComponent* MovementComponent = Character ? Character->GetTalesCharacterMovement() : nullptr;
	bool res = false;
	if(MovementComponent)
	{
		const FVector UnrotateLastInputVector = UKismetMathLibrary::Quat_UnrotateVector(Character->GetActorRotation().Quaternion(), MovementComponent->GetLastUpdateAcceleration());
		const float DotResult = FVector::DotProduct(UnrotateLastInputVector.GetSafeNormal(), FVector(0.f, 0.f, 1.f));
		const float DotResultLeft = FVector::DotProduct(UnrotateLastInputVector.GetSafeNormal(), FVector(0.f, 1.f, 0.f));
		if(DotResult > 0.8f)
		{
			HopMontage = AnimMontages.FDirectionMontage;
			res = MovementComponent->CheckCanHopUp(100.f, -30.f, 150.f);
		}else if(DotResult < -0.8f)
		{
			HopMontage = AnimMontages.BDirectionMontage;
			res = MovementComponent->CheckCanHopUp(100.f, -30.f, -150.f);
		}else if(DotResultLeft > 0.8f)
		{
			HopMontage = AnimMontages.RDirectionMontage;
			res = MovementComponent->CheckCanHopUp(100.f, -30.f, 150.f);
		}else if(DotResultLeft < -0.8f)
		{
			HopMontage = AnimMontages.LDirectionMontage;
			res = MovementComponent->CheckCanHopUp(100.f, -30.f, 150.f);
		}else if(DotResult > 0.f && DotResultLeft > 0.f)
		{
			HopMontage = AnimMontages.FRDirectionMontage;
			res = MovementComponent->CheckCanHopUp(100.f, -30.f, 150.f);
		}else if(DotResult > 0.f && DotResultLeft < 0.f)
		{
			HopMontage = AnimMontages.FLDirectionMontage;
			res = MovementComponent->CheckCanHopUp(100.f, -30.f, 150.f);
		}else if(DotResult < 0.f && DotResultLeft > 0.f)
		{
			HopMontage = AnimMontages.BRDirectionMontage;
			res = MovementComponent->CheckCanHopUp(100.f, -30.f, -150.f);
		}else
		{
			HopMontage = AnimMontages.BLDirectionMontage;
			res = MovementComponent->CheckCanHopUp(100.f, -30.f, -150.f);
		}
	}
	return res;
}
