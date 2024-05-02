// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "System/TalesRunnerTypes.h"
#include "InventoryList.generated.h"

class UInventoryItemInstance;

USTRUCT(BlueprintType)
struct FInventoryListItem : public FFastArraySerializerItem
{
	GENERATED_USTRUCT_BODY()
	
	UPROPERTY()
	UInventoryItemInstance* ItemInstance = nullptr;
};

USTRUCT(BlueprintType)
struct FInventoryList : public FFastArraySerializer
{
	GENERATED_USTRUCT_BODY()

	TArray<FInventoryListItem>& GetItemsRef(){ return Items; }
	
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FInventoryListItem, FInventoryList>(Items, DeltaParams, *this);
	}

	void AddItem(TSubclassOf<UItemStaticData> InItemStaticDataClass);
	void AddItem(UInventoryItemInstance* InItemInstance);
	void RemoveItem(TSubclassOf<UItemStaticData> InItemStaticDataClass);
	
protected:
	UPROPERTY()
	TArray<FInventoryListItem> Items;
};

template<>
struct TStructOpsTypeTraits<FInventoryList> : public  TStructOpsTypeTraitsBase2<FInventoryList>
{
	enum
	{
		WithNetDeltaSerializer         = true,                         // struct has a NetDeltaSerialize function for serializing differences in state from a previous NetSerialize operation.
	};
};
