// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayCueInterface.h"
#include "GameplayTagAssetInterface.h"
#include "GameFramework/Character.h"
#include "System/TalesRunnerTypes.h"
#include "TalesCharacterBase.generated.h"

class UTalesPawnExtensionComponent;
class UTalesAbilitySystemCompBase;

/** The type we use to send FastShared movement updates. */
USTRUCT()
struct FSharedRepMovement
{
	GENERATED_BODY()

	FSharedRepMovement();

	bool FillForCharacter(ACharacter* Character);
	bool Equals(const FSharedRepMovement& Other, ACharacter* Character) const;

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	UPROPERTY(Transient)
	FRepMovement RepMovement;

	UPROPERTY(Transient)
	float RepTimeStamp = 0.0f;

	UPROPERTY(Transient)
	uint8 RepMovementMode = 0;
	
	UPROPERTY(Transient)
	bool bProxyIsJumpForceApplied = false;

	UPROPERTY(Transient)
	bool bIsCrouched = false;
};


template<>
struct TStructOpsTypeTraits<FSharedRepMovement> : public TStructOpsTypeTraitsBase2<FSharedRepMovement>
{
	enum
	{
		WithNetSerializer = true,
		WithNetSharedSerialization = true,
	};
};

/*!
 * @name ATalesCharacterBase
 *
 * @note the base character pawn class used by this project
 * Responsible for sending to pawn component.
 * New Behavior should be added via pawn comp when possible
 */
UCLASS(Config = Game, meta = (ShortToolTip = "The base character pawn class used by this project. "))
class D_TALESRUNNER_REP_API ATalesCharacterBase : public ACharacter, public IGameplayTagAssetInterface, public IGameplayCueInterface , public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ATalesCharacterBase(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Tales|Character")
	UTalesAbilitySystemCompBase* GetTalesAbilitySystemComponent() const;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;
	virtual bool HasMatchingGameplayTag(FGameplayTag TagToCheck) const override;
	virtual bool HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;
	virtual bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;

	//! @brief Actor interface
	//! Begin AActor Interface: Modular Gameplay
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void Reset() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;
	//! End AActor Interface

protected:
	virtual void OnRep_PlayerState() override;
	
	void DisableMovementAndCollision();
	void InitializeGameplayTags();

	void SetMovementModeTag(EMovementMode MovementMode, uint8 CustomMovementMode, bool bTagEnable);

	void UnInitAndDestory();

private:
	virtual void OnAbilitySystemInitialized();
	virtual void OnAbilitySystemUnInitialized();
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Teals|Character", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTalesPawnExtensionComponent> PawnExtComponent;

	UPROPERTY(Transient, ReplicatedUsing = OnRep_ReplicatedAcceleration)
	FTalesReplicatedAcceleration ReplicatedAcceleration;
	
private:
	UFUNCTION()
	void OnRep_ReplicatedAcceleration();
};

