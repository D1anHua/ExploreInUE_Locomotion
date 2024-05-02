// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/Data/InventoryItemInstance.h"

#include "Net/UnrealNetwork.h"
#include "System/TalesBlueprintLibrary.h"

void UInventoryItemInstance::Init(TSubclassOf<UItemStaticData> InItemStaticDataClass)
{
	ItemStaticDataClass = InItemStaticDataClass;
}

const UItemStaticData* UInventoryItemInstance::GetItemStaticData() const
{
	return UTalesBlueprintLibrary::GetItemStaticData(ItemStaticDataClass);
}

void UInventoryItemInstance::OnRep_Equipped()
{
}

void UInventoryItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	UObject::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInventoryItemInstance, ItemStaticDataClass);
	DOREPLIFETIME(UInventoryItemInstance, bEquipped);
}

void UInventoryItemInstance::OnEquipped(AActor* InOwner)
{
}

void UInventoryItemInstance::OnUnEquipped(AActor* InOwner)
{
}

void UInventoryItemInstance::OnDropped(AActor* InOwner)
{
}
