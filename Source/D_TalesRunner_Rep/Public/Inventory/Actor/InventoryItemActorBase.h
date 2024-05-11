// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "System/TalesRunnerTypes.h"
#include "InventoryItemActorBase.generated.h"

class UInventoryItemInstance;

UCLASS()
class D_TALESRUNNER_REP_API AInventoryItemActorBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AInventoryItemActorBase();

	virtual void OnEquipped();
	virtual void OnUnequipped();
	virtual void OnDropped();
	virtual void OnUsed();
	void Init(UInventoryItemInstance* InInstance);
	
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(ReplicatedUsing = OnRep_ItemInstance)
	UInventoryItemInstance* ItemInstance = nullptr;
	UFUNCTION()
	void OnRep_ItemInstance(UInventoryItemInstance* OldItemInstance);

	UPROPERTY(ReplicatedUsing = OnRep_ItemState)
	EItemState ItemState = EItemState::None;
	UFUNCTION()
	void OnRep_ItemState();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class USphereComponent* SphereComponent = nullptr;
	
	UFUNCTION()
	void OnSphereOverlay(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<UItemStaticData> ItemStaticDataClass;

	virtual void InitInternal();

	UPROPERTY(EditDefaultsOnly)
	FName AttachmentSocket = NAME_None;
	// 注: 这两个选项一般不能同时存在
	//! 是否可以被装备, 例如: 武器, 盾牌
	bool bCanEquipped = false;
	//! 是否可以使用, 例如: 食物
	bool bCanUsed = false;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
