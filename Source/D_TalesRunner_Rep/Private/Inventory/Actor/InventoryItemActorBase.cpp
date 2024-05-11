// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/Actor/InventoryItemActorBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "System/TalesLogChannels.h"
#include "Components/SphereComponent.h"
#include "Engine/ActorChannel.h"
#include "Inventory/TalesInventoryComponent.h"
#include "Inventory/Data/InventoryItemInstance.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"

AInventoryItemActorBase::AInventoryItemActorBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SphereComponent->SetupAttachment(RootComponent);
	SphereComponent->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnSphereOverlay);
}

void AInventoryItemActorBase::Init(UInventoryItemInstance* InInstance)
{
	ItemInstance = InInstance;
}

void AInventoryItemActorBase::OnEquipped()
{
	// if(bCanEquipped)
	// {
		ItemState = EItemState::Equipped;
		
		SphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SphereComponent->SetGenerateOverlapEvents(false);
	// }
	// else
	// {
	// 	UE_LOG(LogInventory, Error, TEXT("企图装备不能装备的Actor: %s"), *ItemInstance->GetItemStaticData()->ItemRowHandle.RowName.ToString());
	// 	UE_LOG(LogInventory, Error, TEXT("Attempt to equip Actor: %s, which bCanEquipped is false"), *ItemInstance->GetItemStaticData()->ItemRowHandle.RowName.ToString());
	// }
}

void AInventoryItemActorBase::OnUnequipped()
{
	if(bCanEquipped)
	{
		ItemState = EItemState::None;

		SphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SphereComponent->SetGenerateOverlapEvents(false);
	}
}

void AInventoryItemActorBase::OnDropped()
{
	ItemState = EItemState::Dropped;

	GetRootComponent()->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

	// 这部分也可以用物理材料的方式模拟
	if(AActor* ActorOwner = GetOwner())
	{
		const FVector Location = GetActorLocation();
		const FVector Forward = ActorOwner->GetActorForwardVector();

		const float DropItemDist = 100.f;
		const float DropItemTraceDist = 1000.f;

		const FVector TraceStart = Location + Forward * DropItemDist;
		const FVector TraceEnd = TraceStart - FVector::UpVector * DropItemTraceDist;

		TArray<AActor*> ActorsToIgnore = {GetOwner()};

		FHitResult TraceHit;
		// Cheat Debug
		static const auto CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ShowDebugInventory"));
		const bool bShowInventory = CVar->GetInt() > 0;

		FVector TargetLocation = TraceEnd;

		EDrawDebugTrace::Type DebugDrawType = bShowInventory ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;
		if(UKismetSystemLibrary::LineTraceSingleByProfile(this, TraceStart, TraceEnd, TEXT("WorldStatic"), true, ActorsToIgnore, DebugDrawType, TraceHit, true))
		{
			if(TraceHit.bBlockingHit)
			{
				TargetLocation = TraceHit.Location;
			}
		}
		SetActorLocation(TargetLocation);
	}

	SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComponent->SetGenerateOverlapEvents(true);
}

void AInventoryItemActorBase::OnUsed()
{
	if(bCanUsed)
	{
		// @Todo
		SphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SphereComponent->SetGenerateOverlapEvents(false);
	}
	else
	{
		UE_LOG(LogInventory, Error, TEXT("企图使用不能使用的Actor: %s"), *ItemInstance->GetItemStaticData()->ItemRowHandle.RowName.ToString());
		UE_LOG(LogInventory, Error, TEXT("Attempt to equip Actor: %s, which bCanUsed is false"), *ItemInstance->GetItemStaticData()->ItemRowHandle.RowName.ToString());
	}
}


bool AInventoryItemActorBase::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
	WroteSomething |= Channel->ReplicateSubobject(ItemInstance, *Bunch, *RepFlags);
	return WroteSomething;
}

void AInventoryItemActorBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AInventoryItemActorBase, ItemInstance);
	DOREPLIFETIME(AInventoryItemActorBase, ItemState);
}

void AInventoryItemActorBase::OnRep_ItemInstance(UInventoryItemInstance* OldItemInstance)
{
	if(IsValid(ItemInstance) && !IsValid(OldItemInstance))
	{
		bCanEquipped = ItemInstance->GetItemStaticData()->CanEquipped();
		bCanUsed = ItemInstance->GetItemStaticData()->CanUsed();
		AttachmentSocket = ItemInstance->GetItemStaticData()->GetAttackSocket();
		InitInternal();
	}
}

void AInventoryItemActorBase::OnRep_ItemState()
{
	switch (ItemState)
	{
	case EItemState::Equipped:
	case EItemState::None:
		SphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SphereComponent->SetGenerateOverlapEvents(false);
		break;
	default:
		SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		SphereComponent->SetGenerateOverlapEvents(true);
		break;
	}
}

void AInventoryItemActorBase::OnSphereOverlay(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if(HasAuthority())
	{
		FGameplayEventData EventPayload;
		EventPayload.Instigator = this;
		EventPayload.OptionalObject = ItemInstance;
		EventPayload.EventTag = UTalesInventoryComponent::PickItemActorTag;

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OtherActor, UTalesInventoryComponent::PickItemActorTag, EventPayload);
	}
}

void AInventoryItemActorBase::InitInternal()
{
}

// Called when the game starts or when spawned
void AInventoryItemActorBase::BeginPlay()
{
	Super::BeginPlay();

	if(HasAuthority())
	{
		if(!IsValid(ItemInstance) && IsValid(ItemStaticDataClass))
		{
			ItemInstance = NewObject<UInventoryItemInstance>();
			ItemInstance->Init(ItemStaticDataClass);

			SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			SphereComponent->SetGenerateOverlapEvents(true);

			InitInternal();
		}
	}
	
}

// Called every frame
void AInventoryItemActorBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

