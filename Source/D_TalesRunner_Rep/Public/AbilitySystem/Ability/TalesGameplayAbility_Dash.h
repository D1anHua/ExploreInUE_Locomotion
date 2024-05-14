// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/TalesGameplayAbility.h"
#include "TalesGameplayAbility_Dash.generated.h"

UCLASS()
class D_TALESRUNNER_REP_API UTalesGameplayAbility_Dash : public UTalesGameplayAbility
{
	GENERATED_BODY()

public:
	UTalesGameplayAbility_Dash();

	virtual bool CommitCheck(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, FGameplayTagContainer* OptionalRelevantTags) override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
protected:
	UPROPERTY(EditDefaultsOnly)
	FFourDirectionMontageAnim AnimMontages;

	UPROPERTY()
	class UAbilityTask_PlayMontageAndWait* MontageTask = nullptr;
	
private:
	UPROPERTY(Transient)
	UAnimMontage* DashMontage = nullptr;
	
	virtual void RequestDashDirection();
};
