// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/TalesInventoryComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayTagsManager.h"
#include "Character/TalesCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Engine/ActorChannel.h"
#include "Inventory/TalesInventorInteractUI.h"
#include "Inventory/TalesInventoryInterface.h"
#include "Inventory/TalesInventoryUserWidget.h"
#include "Inventory/Actor/TalesMoney.h"

#include "Inventory/Data/InventoryItemInstance.h"

#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

FGameplayTag UTalesInventoryComponent::PickItemActorTag;
FGameplayTag UTalesInventoryComponent::EquipTag;
FGameplayTag UTalesInventoryComponent::UnEquipTag;
FGameplayTag UTalesInventoryComponent::DropItemTag;

// Cheat Debug Config
static TAutoConsoleVariable<int32> CVarDebugInventory(
	TEXT("ShowDebugInventory"),
	0,
	TEXT("Draws debug info about inventory")
	TEXT("0: Off/n")
	TEXT("1: On/n"),
	ECVF_Cheat
);

// Sets default values for this component's properties
UTalesInventoryComponent::UTalesInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
	
	InventoryMoneyAmount = 0;
	InventoryMaxHeart = 8;
	InventoryHeartNow = 7.5;

	// Gameplay Ability System
	static bool bHandleAddingTags = false;
	if(!bHandleAddingTags)
	{
		bHandleAddingTags = true;
		UGameplayTagsManager::Get().OnLastChanceToAddNativeTags().AddUObject(this, &UTalesInventoryComponent::AddInventoryTags);
	}

}

void UTalesInventoryComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

void UTalesInventoryComponent::PostLoad()
{
	Super::PostLoad();
}

bool UTalesInventoryComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool WroteSomething =  Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
	for(FInventoryListItem& Item : InventoryList.GetItemsRef())
	{
		UInventoryItemInstance* ItemInstance = Item.ItemInstance;
		if(IsValid(ItemInstance))
		{
			WroteSomething |= Channel->ReplicateSubobject(ItemInstance, *Bunch, *RepFlags);
		}
	}
	
	for(FInventoryListItem& Item : SwardList.GetItemsRef())
	{
		UInventoryItemInstance* ItemInstance = Item.ItemInstance;
		if(IsValid(ItemInstance))
		{
			WroteSomething |= Channel->ReplicateSubobject(ItemInstance, *Bunch, *RepFlags);
		}
	}
	
	for(FInventoryListItem& Item : ShieldList.GetItemsRef())
	{
		UInventoryItemInstance* ItemInstance = Item.ItemInstance;
		if(IsValid(ItemInstance))
		{
			WroteSomething |= Channel->ReplicateSubobject(ItemInstance, *Bunch, *RepFlags);
		}
	}
	
	for(FInventoryListItem& Item : EatableList.GetItemsRef())
	{
		UInventoryItemInstance* ItemInstance = Item.ItemInstance;
		if(IsValid(ItemInstance))
		{
			WroteSomething |= Channel->ReplicateSubobject(ItemInstance, *Bunch, *RepFlags);
		}
	}
	return WroteSomething;
}

// Called when the game starts
void UTalesInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	TalesCharacterOwner = Cast<ATalesCharacter>(GetOwner());
	if(TalesCharacterOwner)
	{
		auto CapsuleComp = TalesCharacterOwner->GetCapsuleComponent();

		// @Todo: 后面需要删除这一部分: 不支持网络同步的数据结构
		CapsuleComp->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnCharacterOverlap);
		SetSwardSlot(this->OnUseSwardSlot);
		SetShieldSlot(this->OnUseShieldSlot);
	}

	if(ensureAlways(InventoryWidgetClass) && GetOwnerRole() == ROLE_AutonomousProxy)
	{
		 InventoryWidget = CreateWidget<UTalesInventoryUserWidget>(UGameplayStatics::GetPlayerController(GetWorld(), 0), InventoryWidgetClass);
	}
	
	if(UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner()))
	{
		ASC->GenericGameplayEventCallbacks.FindOrAdd(UTalesInventoryComponent::PickItemActorTag).AddUObject(this, &ThisClass::GameplayEventCallback);
		ASC->GenericGameplayEventCallbacks.FindOrAdd(UTalesInventoryComponent::DropItemTag).AddUObject(this, &ThisClass::GameplayEventCallback);
		ASC->GenericGameplayEventCallbacks.FindOrAdd(UTalesInventoryComponent::EquipTag).AddUObject(this, &ThisClass::GameplayEventCallback);
		ASC->GenericGameplayEventCallbacks.FindOrAdd(UTalesInventoryComponent::UnEquipTag).AddUObject(this, &ThisClass::GameplayEventCallback);
	}

#if WITH_EDITORONLY_DATA
	if(GetOwner()->HasAuthority())
	{
		for(const auto ItemClass : DefaultItems)
		{
			InventoryList.AddItem(ItemClass);
		}
	}
#endif
}

void UTalesInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// @TODO 后续添加到Actor中
	PrimaryInteractTraceByFoot();

	// Debug Cheat
	const bool bShowDebug = CVarDebugInventory.GetValueOnAnyThread() != 0;
	if(bShowDebug)
	{
		for(FInventoryListItem& Item : InventoryList.GetItemsRef())
		{
			UInventoryItemInstance* ItemInstance = Item.ItemInstance;
			const UItemStaticData* ItemStaticData = ItemInstance->GetItemStaticData();
			if(IsValid(ItemInstance) && IsValid(ItemStaticData))
			{
				GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Blue, FString::Printf(TEXT("Item: %s"), *ItemStaticData->ItemRowHandle.RowName.ToString()));
			}
		}
	}
}

void UTalesInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UTalesInventoryComponent, InventoryList);
	DOREPLIFETIME(UTalesInventoryComponent, SwardList);
	DOREPLIFETIME(UTalesInventoryComponent, ShieldList);
	DOREPLIFETIME(UTalesInventoryComponent, EatableList);
	DOREPLIFETIME(UTalesInventoryComponent, CurrentShieldItem);
	DOREPLIFETIME(UTalesInventoryComponent, CurrentSwardItem);
}

void UTalesInventoryComponent::SetSwardSlot(FTalesInventoryItemSlot NewSwardSlot)
{
	OnUseSwardSlot = NewSwardSlot;
	if(OnUseSwardSlot.IsValid() && OnUseSwardSlot.ItemType == Sward)
	{
		auto Row = OnUseSwardSlot.GetRow();
		if(Row)
		{
			TalesCharacterOwner->SetSwardMesh(Row->Mesh);
		}
	}else if(!OnUseSwardSlot.IsValid())
	{
		// 拆掉Mesh
		TalesCharacterOwner->SetSwardMesh(nullptr);
	}
}

void UTalesInventoryComponent::SetShieldSlot(FTalesInventoryItemSlot NewShieldSlot)
{
	OnUseShieldSlot = NewShieldSlot;
	if(OnUseShieldSlot.IsValid() && OnUseShieldSlot.ItemType == Shield)
	{
		auto Row = OnUseShieldSlot.GetRow();
		if(Row)
		{
			TalesCharacterOwner->SetShieldMesh(Row->Mesh);
		}
	}
	else if(!OnUseShieldSlot.IsValid())
	{
		TalesCharacterOwner->SetShieldMesh(nullptr);
	}
}

bool UTalesInventoryComponent::PackageDataDecrease(FTalesInventoryItemSlot TargetName, int delta)
{
	if(delta > 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Inventory Component : Package Data Changes Doesn't support delta > 0"))
		return false;
	}
	auto ItemType = TargetName.ItemType;
	TMultiMap<FName, FTalesInventoryItemSlot>* PickedColumnItems;
	switch(ItemType)
	{
	case Sward:
		PickedColumnItems = &PackageDatas.Sward;
		break;
	case Shield:
		PickedColumnItems = &PackageDatas.Shield;
		break;
	case Eatable:
		TargetName.Quantity = delta;
		PackageDataChange_Eatable(TargetName);
		return true;
	default:
		return false;			
	}
	// 只对 sward 和 shield
	auto FindAns = PickedColumnItems->Find(TargetName.ItemRowHandle.RowName);
	while(FindAns != nullptr &&  FindAns->ItemRowHandle.RowName == TargetName.ItemRowHandle.RowName)
	{
		if(*FindAns == TargetName)
		{
			PickedColumnItems->RemoveSingle(TargetName.ItemRowHandle.RowName, TargetName);
			return true;
		}
		else
		{
			++FindAns;
		}
	}
	return false;
}
// ------------------------------------------------  Main Data -------------------------------------------------
void UTalesInventoryComponent::AddItemToPackage(AActor* HitActor)
{
	auto ItemActor = Cast<ATalesInventoryItem>(HitActor);
	if(ItemActor)
	{
		TMultiMap<FName ,FTalesInventoryItemSlot>* PickedColumnItems;
		switch(ItemActor->Item.ItemType)
		{
		case Sward:
			PickedColumnItems = &PackageDatas.Sward;
			break;
		case Shield:
			PickedColumnItems = &PackageDatas.Shield;
			break;
		case Eatable:
			PickedColumnItems = &PackageDatas.Eatable;
			PackageDataChange_Eatable(ItemActor->Item);
			return;
		default:
			return;			
		}
		// Add Item, Just Sward and Shield
		// 注: Sward 不合并 quantity, 直接装
		// Sward和shield没有 Stack的概念
		PickedColumnItems->AddUnique(ItemActor->Item.ItemRowHandle.RowName, ItemActor->Item);	
	}
}

void UTalesInventoryComponent::PackageDataChange_Eatable(const FTalesInventoryItemSlot ItemEatable)
{
		auto FindAns = PackageDatas.Eatable.Find(ItemEatable.ItemRowHandle.RowName);
		if(FindAns == nullptr)
		{
			if(ItemEatable.IsValid())
			{
				// 原先没有
				PackageDatas.Eatable.AddUnique(ItemEatable.ItemRowHandle.RowName, ItemEatable);	
			}
		}
		else
		{
			// 不考虑装满不装满的问题了
			int numTemp = FindAns->Quantity + ItemEatable.Quantity;
			FindAns->Quantity = numTemp;
			if(FindAns->Quantity <= 0)
			{
				PackageDatas.Eatable.RemoveSingle(ItemEatable.ItemRowHandle.RowName, *FindAns);	
			}
		}
}

void UTalesInventoryComponent::AddItem(TSubclassOf<UItemStaticData> InItemStaticDataClass)
{
	if(GetOwner()->HasAuthority())
	{
		switch (InItemStaticDataClass.GetDefaultObject()->ItemType)
		{
		case SwardItem:
			SwardList.AddItem(InItemStaticDataClass);
			break;
		case ShieldItem:
			ShieldList.AddItem(InItemStaticDataClass);
			break;
		case EatableItem:
			EatableList.AddItem(InItemStaticDataClass);
			break;
		default:
			break;
		}
		InventoryList.AddItem(InItemStaticDataClass);
	}
}

void UTalesInventoryComponent::AddItemInstance(UInventoryItemInstance* InItemInstance)
{
	if(GetOwner()->HasAuthority())
	{
		switch (InItemInstance->GetItemStaticData()->ItemType)
		{
		case SwardItem:
			SwardList.AddItem(InItemInstance);
			break;
		case ShieldItem:
			ShieldList.AddItem(InItemInstance);
			break;
		case EatableItem:
			EatableList.AddItem(InItemInstance);
			break;
		default:
			break;
		}
		InventoryList.AddItem(InItemInstance);
	}
}

void UTalesInventoryComponent::RemoveItem(TSubclassOf<UItemStaticData> InItemStaticDataClass)
{
	if(GetOwner()->HasAuthority())
	{
		switch (InItemStaticDataClass.GetDefaultObject()->ItemType)
		{
		case SwardItem:
			SwardList.AddItem(InItemStaticDataClass);
			break;
		case ShieldItem:
			ShieldList.AddItem(InItemStaticDataClass);
			break;
		case EatableItem:
			EatableList.AddItem(InItemStaticDataClass);
			break;
		default:
			break;
		}
		InventoryList.RemoveItem(InItemStaticDataClass);
	}
}

void UTalesInventoryComponent::EquipItem(TSubclassOf<UItemStaticData> InItemStaticDataClass)
{
	if(GetOwner()->HasAuthority())
	{
		for(auto Item : InventoryList.GetItemsRef())
		{
			if(Item.ItemInstance->ItemStaticDataClass == InItemStaticDataClass)
			{
				Item.ItemInstance->OnEquipped(GetOwner());
				CurrentItem = Item.ItemInstance;
				break;
			}
		}
		
		FInventoryList* List = nullptr;
		switch (InItemStaticDataClass.GetDefaultObject()->ItemType)
		{
		case SwardList:
			List = &SwardList;
			break;
		case ShieldList:
			List = &ShieldList;
			break;
		default:
			break;
		}
		if(List != nullptr)
		{
			for(auto Item : List->GetItemsRef())
			{
				if(Item.ItemInstance->ItemStaticDataClass == InItemStaticDataClass)
				{
					Item.ItemInstance->OnEquipped(GetOwner());
					CurrentItem = Item.ItemInstance;
					break;
				}
			}
		}
	}
}

void UTalesInventoryComponent::EquipItemInstance(UInventoryItemInstance* InItemInstance)
{
	if(GetOwner()->HasAuthority())
	{
		for(auto Item : InventoryList.GetItemsRef())
		{
			if(Item.ItemInstance == InItemInstance)
			{
				Item.ItemInstance->OnEquipped(GetOwner());
				CurrentItem = Item.ItemInstance;
				break;
			}
		}
		
		FInventoryList* List = nullptr;
		switch (InItemInstance->GetItemStaticData()->ItemType)
		{
		case SwardList:
			List = &SwardList;
			break;
		case ShieldList:
			List = &ShieldList;
			break;
		default:
			break;
		}
		if(List != nullptr)
		{
			for(auto Item : List->GetItemsRef())
			{
				if(Item.ItemInstance == InItemInstance)
				{
					Item.ItemInstance->OnEquipped(GetOwner());
					CurrentItem = Item.ItemInstance;
					break;
				}
			}
		}
	}
}

void UTalesInventoryComponent::UnEquipItem()
{
	if(GetOwner()->HasAuthority())
	{
		if(IsValid(CurrentItem))
		{
			CurrentItem->OnUnEquipped(GetOwner());
			CurrentItem = nullptr;
		}
	}
}

void UTalesInventoryComponent::DropItem()
{
	if(GetOwner()->HasAuthority())
	{
		if(IsValid(CurrentItem))
		{
			CurrentItem->OnDropped(GetOwner());
			RemoveItem(CurrentItem->ItemStaticDataClass);
			CurrentItem = nullptr;
		}
	}
}

void UTalesInventoryComponent::EquipItemNext()
{
	TArray<FInventoryListItem>& Items = InventoryList.GetItemsRef();
	const bool bNoItems = Items.Num() == 0;
	const bool bOneAndEquipped = Items.Num() == 1 && CurrentItem;

	if(bNoItems || bOneAndEquipped) return;

	UInventoryItemInstance* TargetItem = CurrentItem;
	for(auto Item : Items)
	{
		if(Item.ItemInstance->GetItemStaticData()->CanEquipped())
		{
			if(Item.ItemInstance != CurrentItem)
			{
				TargetItem = Item.ItemInstance;
				break;
			}
		}
	}

	if(CurrentItem)
	{
		if(TargetItem == CurrentItem)
		{
			return;
		}
		UnEquipItem();
	}
	EquipItemInstance(TargetItem);
}

void UTalesInventoryComponent::GameplayEventCallback(const FGameplayEventData* PayLoad)
{
	ENetRole NetRole = GetOwnerRole();
	if(NetRole == ROLE_Authority)
	{
		HandleGameplayEventInternal(*PayLoad);
	}
	else if(NetRole == ROLE_AutonomousProxy)
	{
		ServerHandleGameplayEvent(*PayLoad);
	}
}

void UTalesInventoryComponent::AddInventoryTags()
{
	UGameplayTagsManager& TagsManager = UGameplayTagsManager::Get();

	UTalesInventoryComponent::PickItemActorTag = TagsManager.AddNativeGameplayTag(TEXT("Event.Inventory.PickItemActor"), TEXT("Pick Item from item actor event"));
	UTalesInventoryComponent::DropItemTag = TagsManager.AddNativeGameplayTag(TEXT("Event.Inventory.DropItem"), TEXT("Drop equipped item"));
	UTalesInventoryComponent::EquipTag = TagsManager.AddNativeGameplayTag(TEXT("Event.Inventory.EquipItem"), TEXT("Try equip item"));
	UTalesInventoryComponent::UnEquipTag = TagsManager.AddNativeGameplayTag(TEXT("Event.Inventory.UnEquip"), TEXT("Try unequip item"));
	TagsManager.OnLastChanceToAddNativeTags().RemoveAll(this);
}

void UTalesInventoryComponent::HandleGameplayEventInternal(FGameplayEventData Payload)
{
	ENetRole NetRole = GetOwnerRole();
	if(NetRole == ROLE_Authority)
	{
		FGameplayTag EventTag = Payload.EventTag;

		if(EventTag == UTalesInventoryComponent::PickItemActorTag)
		{
			if(const UInventoryItemInstance* ItemInstance = Cast<UInventoryItemInstance>(Payload.OptionalObject))
			{
				AddItemInstance(const_cast<UInventoryItemInstance*>(ItemInstance));

				if(Payload.Instigator)
				{
					const_cast<AActor*>(Payload.Instigator.Get())->Destroy();
				}
			}
		}
		else if(EventTag == UTalesInventoryComponent::EquipTag)
		{
			EquipItemNext();
		}
		else if(EventTag == UTalesInventoryComponent::DropItemTag)
		{
			DropItem();
		}
		else if(EventTag == UTalesInventoryComponent::UnEquipTag)
		{
			UnEquipItem();
		}
	}
}

void UTalesInventoryComponent::ServerHandleGameplayEvent_Implementation(FGameplayEventData Payload)
{
	HandleGameplayEventInternal(Payload);
}

void UTalesInventoryComponent::PickKeyPressed()
{
	// @TODO Delete, 做两遍检测
	FHitResult Hit;
	FCollisionShape CollisionShape;
	CollisionShape.SetBox(FVector3f(40.f, 40.f, 20.f));
	FVector BeginLocation = TalesCharacterOwner->GetActorLocation() -  TalesCharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * TalesCharacterOwner->GetActorUpVector();
	FVector EndLocation = BeginLocation + TalesCharacterOwner->GetActorForwardVector() * 60;
	bool bGetHit = GetWorld()->SweepSingleByProfile(Hit, BeginLocation, EndLocation, FQuat::Identity, "InventoryItem", CollisionShape,TalesCharacterOwner->GetIgnoreCharacterParams());
	if(bGetHit && Hit.GetActor()->Implements<UTalesInventoryInterface>())
	{
		AddItemToPackage(Hit.GetActor());
		Hit.GetActor()->Destroy();
	}
}

void UTalesInventoryComponent::PrimaryInteractTraceBySight()
{
	FVector EyeLocation;
	FRotator EyeRotation;
	TalesCharacterOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

	FVector End = EyeLocation + (EyeRotation.Vector() * 1000);

	FHitResult Hit;
	FCollisionShape CollisionShape;
	CollisionShape.SetSphere(20.f);
	bool bGetHit = GetWorld()->SweepSingleByProfile(Hit, EyeLocation, End, FQuat::Identity, "InventoryItem", CollisionShape,TalesCharacterOwner->GetIgnoreCharacterParams());
	if(bGetHit)
	{
		AActor* HitActor = Hit.GetActor();
		if(HitActor->Implements<UTalesInventoryInterface>())
		{
			APawn* MyPawn = Cast<APawn>(TalesCharacterOwner);
			ITalesInventoryInterface::Execute_Interact(HitActor, MyPawn);	
		}
		else
		{
			if(TalesCharacterOwner->InteractUI->IsInViewport())
			{
				TalesCharacterOwner->UnActivateInteractUI();
			}
		}
		// DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 25.f, 32, FColor::Green, false, 4.f);
	}
	// FColor DebugColor = bGetHit ? FColor::Green : FColor::Red;
	// DrawDebugLine(GetWorld(), EyeLocation, End, DebugColor, false, 2.f, 0, 2.f);	
}

void UTalesInventoryComponent::PrimaryInteractTraceByFoot()
{
	FHitResult Hit;
	FCollisionShape CollisionShape;
	CollisionShape.SetBox(FVector3f(40.f, 40.f, 20.f));
	FVector BeginLocation = TalesCharacterOwner->GetActorLocation() -  TalesCharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * TalesCharacterOwner->GetActorUpVector();
	FVector EndLocation = BeginLocation + TalesCharacterOwner->GetActorForwardVector() * 60;
	bool bGetHit = GetWorld()->SweepSingleByProfile(Hit, BeginLocation, EndLocation, FQuat::Identity, "InventoryItem", CollisionShape,TalesCharacterOwner->GetIgnoreCharacterParams());
	if(bGetHit && Hit.GetActor()->Implements<UTalesInventoryInterface>())
	{
		AActor* HitActor = Hit.GetActor();
		APawn* MyPawn = Cast<APawn>(TalesCharacterOwner);
		ITalesInventoryInterface::Execute_Interact(HitActor, MyPawn);	
		// DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 25.f, 32, FColor::Green, false, 4.f);
	}
	else
	{
		if(TalesCharacterOwner->InteractUI && TalesCharacterOwner->InteractUI->IsInViewport())
		{
			TalesCharacterOwner->UnActivateInteractUI();
		}
	}
	// FColor DebugColor = bGetHit ? FColor::Green : FColor::Red;
	// DrawDebugLine(GetWorld(), BeginLocation, EndLocation, DebugColor, false, 2.f, 0, 2.f);	
}
void UTalesInventoryComponent::OnCharacterOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	auto Money = Cast<ATalesMoney>(OtherActor);
	if(Money)
	{
		// MoneyAmountChangeDelegate.Broadcast(Cast<AActor>(Money), this, InventoryMoneyAmount, Money->MoneyData.Amount);
		Money->Destroy();
		GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Blue, FString::FromInt(InventoryMoneyAmount));
	}
}

