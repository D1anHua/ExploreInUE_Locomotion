// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryItemInstance.generated.h"

/**
 * 
 */
UCLASS()
class D_TALESRUNNER_REP_API UInventoryItemInstance : public UObject
{
	GENERATED_BODY()

public:
	virtual void Init(TSubclassOf<class UItemStaticData> InItemStaticDataClass);
	
	virtual bool IsSupportedForNetworking() const override { return true; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	const UItemStaticData* GetItemStaticData() const;
	
	UPROPERTY(Replicated)
	TSubclassOf<UItemStaticData> ItemStaticDataClass;

	UPROPERTY(ReplicatedUsing = OnRep_Equipped)
	bool bEquipped = false;
	UPROPERTY(ReplicatedUsing = OnRep_Quantity)
	int Quantity = 0;

	UFUNCTION()
	void OnRep_Equipped();
	UFUNCTION()
	void OnRep_Quantity();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void OnEquipped(AActor* InOwner = nullptr);
	virtual void OnUnEquipped(AActor* InOwner = nullptr);
	virtual void OnDropped(AActor* InOwner = nullptr);

protected:
	UPROPERTY(Replicated)
	class AInventoryItemActorBase* ItemActor = nullptr;
};
