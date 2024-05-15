// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/TalesGameplayAbility.h"
#include "TalesGameplayAbility_ClimbHop.generated.h"

/**
 * 
 */
UCLASS()
class D_TALESRUNNER_REP_API UTalesGameplayAbility_ClimbHop : public UTalesGameplayAbility
{
	GENERATED_BODY()

public:
	UTalesGameplayAbility_ClimbHop();

	virtual bool CommitCheck(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, FGameplayTagContainer* OptionalRelevantTags) override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
protected:
	UPROPERTY(EditDefaultsOnly)
	FEightDirectionMontageAnim AnimMontages;

	UPROPERTY()
	class UAbilityTask_PlayMontageAndWait* MontageTask = nullptr;

private:
	UPROPERTY()
	UAnimMontage* HopMontage = nullptr;

	bool RequestHopDirectionAndTest();
};
