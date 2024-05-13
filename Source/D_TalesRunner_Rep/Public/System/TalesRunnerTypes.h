#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "TalesRunnerTypes.generated.h"

// ---------------------------------------------------------------------------------
// -------------------------------- Character Type ---------------------------------
USTRUCT(BlueprintType)
struct FCharacterData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GAS")
	TArray<TSubclassOf<class UGameplayEffect>> Effects;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GAS")
	TArray<TSubclassOf<class UGameplayAbility>> Abilities;

	// UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
	// class UCharacterAnimDataAsset* CharacterAnimDataAsset;
};

UENUM(BlueprintType)
enum EAnimEnumLandState
{
	Normal   UMETA(DisplayName = "NormalLandState"),
	Soft     UMETA(DisplayName = "SoftLandState"),
	Heavy    UMETA(DisplayName = "HeavyLandState"),
};

// ---------------------------------------------------------------------------------
// -------------------------------- Inventory Type ---------------------------------

//! Struct to show message for Item, Data Table
USTRUCT(BlueprintType)
struct FTalesObjectsTableRow : public FTableRowBase
{
	GENERATED_BODY()

	FTalesObjectsTableRow() : Name(FText::FromString("Null")),
								Description(FText::FromString("Nothing")),
								Thumbnails(nullptr),
								StackSize(1),
								Power(0.f),
								Mesh(nullptr){}
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	FText			Name;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	FText			Description;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	UTexture2D*	    Thumbnails;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	int32     	    StackSize;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	float     	    Power;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	UStaticMesh*    Mesh;
};

//! UENUM to display all the entries in the table item, 详细说明就是不同种类的Item
UENUM(BlueprintType)
enum ETalesObjectType
{
	SwardItem      UMETA(DisplayName = "SwardItem"),      //! Sward
	ShieldItem     UMETA(DisplayName = "ShieldItem"),     //! Shield
	EatableItem    UMETA(DisplayName = "EatableItem"),    //! eatable
};

//! 上表中是项目, 下表是对应Item的静态实例
UCLASS(BlueprintType, Blueprintable)
class UItemStaticData : public UObject
{
	GENERATED_BODY()
	
public:
	inline bool IsValid() const { return Quantity > 0; }
	inline FTalesObjectsTableRow* GetRow() const { return ItemRowHandle.GetRow<FTalesObjectsTableRow>("Searching for row..."); }
	
	UPROPERTY(meta = (RowType="TalesObjectsTableRow"), EditAnywhere, BlueprintReadWrite)
	FDataTableRowHandle						   ItemRowHandle;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32									   Quantity = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<ETalesObjectType>			   ItemType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<class AInventoryItemActorBase> ItemActorClass;
	
	inline bool operator==(const UItemStaticData& Others) const { return !(this->ItemRowHandle != Others.ItemRowHandle || this->Quantity != Others.Quantity || this->ItemType != Others.ItemType); }

	bool CanEquipped() const;
	bool CanUsed() const;
	FName GetAttackSocket() const;
};

//! 每个实例对应的状态
UENUM(BlueprintType)
enum class EItemState : uint8
{
	None			UMETA(DisplayName = "NoneState"),
	Equipped		UMETA(DisplayName = "Equipped"),
	Dropped			UMETA(DisplayName = "Dropped")
};
// ----------------------------- End Inventory Type ---------------------------------
// ---------------------------------------------------------------------------------
