// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/Actor/InventoryItemActorBase.h"
#include "EquippedItemActor.generated.h"

/**
 * 
 */
UCLASS()
class D_TALESRUNNER_REP_API AEquippedItemActor : public AInventoryItemActorBase
{
	GENERATED_BODY()

public:
	AEquippedItemActor();
	
protected:
	UPROPERTY()
	UMeshComponent* MeshComponent = nullptr;
	virtual void InitInternal() override;
};
