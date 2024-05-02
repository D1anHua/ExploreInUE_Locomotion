// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/TalesAttributeSetBase.h"
#include "GameplayEffectExtension.h"
#include "Character/TalesCharacter.h"
#include "Character/TalesCharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

// 给定一个初始值, 确保MaxHealth不会大于这个初始值
UTalesAttributeSetBase::UTalesAttributeSetBase()
	:MaxHealth(10000.f), MaxStamina(10000.f), VelocityZoom(1.f), MinVelocityZoom(0.6f), MaxVelocityZoom(1.4f)
{
}

void UTalesAttributeSetBase::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	if(Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));	
	}
	else if(Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		SetStamina(FMath::Clamp(GetStamina(), 0.f, GetMaxStamina()));
	}
	else if(Data.EvaluatedData.Attribute == GetVelocityZoomAttribute())
	{
		ATalesCharacter* OwningCharacter = Cast<ATalesCharacter>(GetOwningActor());
		UTalesCharacterMovementComponent* CharacterMovementComponent = OwningCharacter ? OwningCharacter->GetTalesCharacterMovement() : nullptr;
		if(CharacterMovementComponent)
		{
			CharacterMovementComponent->SetVelocityZoom(FMath::Clamp(GetVelocityZoom(), MinVelocityZoom, MaxVelocityZoom));
		}
	}
}

void UTalesAttributeSetBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UTalesAttributeSetBase, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UTalesAttributeSetBase, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UTalesAttributeSetBase, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UTalesAttributeSetBase, MaxStamina, COND_None, REPNOTIFY_Always);
}

void UTalesAttributeSetBase::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTalesAttributeSetBase, Health, OldHealth);
}

void UTalesAttributeSetBase::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTalesAttributeSetBase, MaxHealth, OldMaxHealth);
}

void UTalesAttributeSetBase::OnRep_Stamina(const FGameplayAttributeData& OldStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTalesAttributeSetBase, Stamina, OldStamina);
}

void UTalesAttributeSetBase::OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTalesAttributeSetBase, MaxStamina, OldMaxStamina);
}

void UTalesAttributeSetBase::OnRep_VelocityZoom(const FGameplayAttributeData& OldVelocityZoom)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTalesAttributeSetBase, MaxStamina, OldVelocityZoom);
}

