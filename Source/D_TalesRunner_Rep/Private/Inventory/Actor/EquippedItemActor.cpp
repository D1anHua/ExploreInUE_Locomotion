// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/Actor/EquippedItemActor.h"

#include "Inventory/Data/InventoryItemInstance.h"

AEquippedItemActor::AEquippedItemActor()
{
	bCanEquipped = true;
	
}

void AEquippedItemActor::InitInternal()
{
	Super::InitInternal();

	if(ItemInstance)
	{
		auto Row = ItemInstance->GetItemStaticData()->GetRow();
		if(Row && Row->Mesh)
		{
			UStaticMeshComponent* StaticMesh = NewObject<UStaticMeshComponent>(this, UStaticMeshComponent::StaticClass(), TEXT("MeshComponent"));
			if(StaticMesh)
			{
				StaticMesh->RegisterComponent();
				StaticMesh->SetStaticMesh(Row->Mesh);
				StaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				StaticMesh->SetGenerateOverlapEvents(false);
				StaticMesh->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);

				MeshComponent = StaticMesh;
			}
		}
		
	}
}
