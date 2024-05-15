// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Ability/TalesGameplayAbility_Climb.h"

#include "Abilities/Tasks/AbilityTask.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/TalesCharacter.h"
#include "Character/TalesCharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"


UTalesGameplayAbility_Climb::UTalesGameplayAbility_Climb()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UTalesGameplayAbility_Climb::CommitCheck(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, FGameplayTagContainer* OptionalRelevantTags)
{
	if(!Super::CommitCheck(Handle, ActorInfo, ActivationInfo, OptionalRelevantTags))
	{
		return false;
	}

	ATalesCharacter* Character = GetTalesCharacterFromActionInfo();
	UTalesCharacterMovementComponent* MovementComponent = Character ? Character->GetTalesCharacterMovement() : nullptr;
	if(MovementComponent == nullptr)
	{
		return false;
	}

	if(MovementComponent->CanClimbUP())
	{
		ClimbMontage = ClimbUpMontage;
		return true;
	}

	if(MovementComponent->CanClimbDown())
	{
		ClimbMontage = ClimbDownMontage;
		return true;
	}
	return false;
}

void UTalesGameplayAbility_Climb::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if(!CommitAbility(Handle, ActorInfo, ActivationInfo) || ClimbMontage == nullptr)
	{
		K2_EndAbility();
		return;
	}
	ATalesCharacter* Character = GetTalesCharacterFromActionInfo();
	UTalesCharacterMovementComponent* MovementComponent = Character ? Character->GetTalesCharacterMovement() : nullptr;
	if(MovementComponent)
	{
		MovementComponent->SetMovementMode(MOVE_Flying);
		MovementComponent->bUseControllerDesiredRotation = false;
		MovementComponent->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = false;
	}
	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, ClimbMontage);

	MontageTask->OnBlendOut.AddDynamic(this, &UTalesGameplayAbility_Climb::EnterClimb);
	MontageTask->OnCompleted.AddDynamic(this, &UTalesGameplayAbility_Climb::EnterClimb);
	MontageTask->OnInterrupted.AddDynamic(this, &UTalesGameplayAbility_Climb::K2_EndAbility);
	MontageTask->OnCancelled.AddDynamic(this, &UTalesGameplayAbility_Climb::K2_EndAbility);
	MontageTask->ReadyForActivation();
}

void UTalesGameplayAbility_Climb::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	ATalesCharacter* Character = GetTalesCharacterFromActionInfo();
	UTalesCharacterMovementComponent* MovementComponent = Character ? Character->GetTalesCharacterMovement() : nullptr;
	if(MovementComponent)
	{
		if(MovementComponent->IsClimbing())
		{
			// 如果是还在climb, 则直接退出
			MovementComponent->SetMovementMode(MOVE_Falling);
		}
		MovementComponent->bUseControllerDesiredRotation = true;
		MovementComponent->bOrientRotationToMovement = true;
		Character->bUseControllerRotationYaw = true;
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UTalesGameplayAbility_Climb::EnterClimb()
{
	if(IsValid(MontageTask))
	{
		MontageTask->EndTask();
	}
	ATalesCharacter* Character = GetTalesCharacterFromActionInfo();
	UTalesCharacterMovementComponent* MovementComponent = Character ? Character->GetTalesCharacterMovement() : nullptr;
	MovementComponent->SetMovementMode(MOVE_Custom, CMOVE_Climb);
}
