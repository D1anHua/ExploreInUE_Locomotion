// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Ability/TalesGameplayAbility_Sprint.h"

#include "Character/TalesCharacter.h"

void UTalesGameplayAbility_Sprint::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	const ATalesCharacter* Character = CastChecked<ATalesCharacter>(ActorInfo->AvatarActor.Get(), ECastCheckedType::NullChecked);
	return Character->Sprint();
}

void UTalesGameplayAbility_Sprint::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	const ATalesCharacter* Character = CastChecked<ATalesCharacter>(ActorInfo->AvatarActor.Get(), ECastCheckedType::NullChecked);
	Character->UnSprint();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
