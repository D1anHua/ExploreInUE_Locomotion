// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Ability/TalesGameplayAbility_Slide.h"

#include "Character/TalesCharacter.h"
#include "Character/TalesCharacterMovementComponent.h"

UTalesGameplayAbility_Slide::UTalesGameplayAbility_Slide()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UTalesGameplayAbility_Slide::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if(Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;	
	}

	GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Blue, "Hello");
	UE_LOG(LogTemp, Error, TEXT("Why"));
	const ATalesCharacter* Character = GetTalesCharacterFromActionInfo();
	const UTalesCharacterMovementComponent* MovementComponent = Character ? Character->GetTalesCharacterMovement() : nullptr;
	if(MovementComponent != nullptr)
	{
		return MovementComponent->CanSlide();
	}
	return false;
}

void UTalesGameplayAbility_Slide::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                                  const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                                  const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Blue, "Hello");
	const ATalesCharacter* Character = GetTalesCharacterFromActionInfo();
	UTalesCharacterMovementComponent* MovementComponent = Character ? Character->GetTalesCharacterMovement() : nullptr;
	MovementComponent->SetMovementMode(MOVE_Custom, CMOVE_Slide);
}

void UTalesGameplayAbility_Slide::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	const ATalesCharacter* Character = GetTalesCharacterFromActionInfo();
	UTalesCharacterMovementComponent* MovementComponent = Character ? Character->GetTalesCharacterMovement() : nullptr;
	MovementComponent->SetMovementMode(MOVE_Walking);
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
