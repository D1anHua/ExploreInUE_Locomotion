// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/Data/InventoryItemInstance.h"

#include "GameFramework/Character.h"
#include "Inventory/Actor/InventoryItemActorBase.h"
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

void UInventoryItemInstance::OnRep_Quantity()
{
}

void UInventoryItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	UObject::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInventoryItemInstance, ItemStaticDataClass);
	DOREPLIFETIME(UInventoryItemInstance, bEquipped);
	DOREPLIFETIME(UInventoryItemInstance, ItemActor);
	DOREPLIFETIME(UInventoryItemInstance, Quantity);
}

void UInventoryItemInstance::OnEquipped(AActor* InOwner)
{
	if(UWorld* World = InOwner->GetWorld())
	{
		const UItemStaticData* StaticData = GetItemStaticData();
		if(StaticData->CanEquipped())
		{
			FTransform Transform;
			ItemActor = World->SpawnActorDeferred<AInventoryItemActorBase>(StaticData->ItemActorClass, Transform, InOwner);
			ItemActor->Init(this);
			ItemActor->OnEquipped();
			ItemActor->FinishSpawning(Transform);

			ACharacter* Character = Cast<ACharacter>(InOwner);
			if(USkeletalMeshComponent* SkeletalMesh =  Character ? Character->GetMesh() : nullptr)
			{
				ItemActor->AttachToComponent(SkeletalMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, StaticData->GetAttackSocket());	
			}
		}
	}
	bEquipped = true;
}

void UInventoryItemInstance::OnUnEquipped(AActor* InOwner)
{
	if(ItemActor)
	{
		ItemActor->Destroy();
		ItemActor = nullptr;
	}
	bEquipped = false;
}

void UInventoryItemInstance::OnDropped(AActor* InOwner)
{
	if(ItemActor)
	{
		ItemActor->OnDropped();
	}
	bEquipped = false;
}
