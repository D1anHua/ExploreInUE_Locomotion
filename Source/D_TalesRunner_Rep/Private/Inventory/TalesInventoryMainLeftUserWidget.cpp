// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/TalesInventoryMainLeftUserWidget.h"

#include "Character/TalesCharacter.h"
#include "Components/Image.h"
#include "Components/WidgetSwitcher.h"
#include "Components/WrapBox.h"
#include "Inventory/UserWidget/TalesSlotUserWidget.h"
#include "Inventory/TalesInventoryComponent.h"
#include "Inventory/UserWidget/TalesItemButton.h"
#include "Kismet/GameplayStatics.h"


void UTalesInventoryMainLeftUserWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	ButtonItems = {SwardButton, ShieldButton, EatableButton};
	ImageItems = {SwardImage, ShieldImage, EatableImage};
	SetTalesCharacterOwner();
	for(auto& item : ButtonItems)
	{
		if(item)
		{
			item->HoverDelegate.AddDynamic(this, &UTalesInventoryMainLeftUserWidget::OnItemButtonHovered);
			item->UnHoverDelegate.AddDynamic(this, &ThisClass::OnItemButtonUnHovered);
			item->ClickDelegate.AddDynamic(this, &ThisClass::OnItemButtonClicked);
		}
	}
}

void UTalesInventoryMainLeftUserWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
}
void UTalesInventoryMainLeftUserWidget::NativeConstruct()
{
	Super::NativeConstruct();
	int Index = PageSwitcher->GetActiveWidgetIndex();
	if(Index != INDEX_NONE)
	{
		ActivateButton(Index);
	}
	InitializeData();
}

void UTalesInventoryMainLeftUserWidget::OnItemButtonHovered(FString Name)
{
	int Index = GetIndex(Name);
	if(Index != INDEX_NONE)
	{
		ImageItems[Index]->SetColorAndOpacity(HoveredColor);		
	}
}

void UTalesInventoryMainLeftUserWidget::OnItemButtonUnHovered(FString Name)
{
	int Index = GetIndex(Name);
	if(Index != INDEX_NONE)
	{
		ImageItems[Index]->SetColorAndOpacity(ActivatedColor);		
	}
}

void UTalesInventoryMainLeftUserWidget::OnItemButtonClicked(FString Name)
{
	int Index = GetIndex(Name);
	if(Index != INDEX_NONE)
	{
		ActivateButton(Index);
	}
}

void UTalesInventoryMainLeftUserWidget::ActivateButton(int Index)
{
	for(int i = 0; i < ButtonItemNames.Num(); ++i)
	{
		if(i == Index)
		{
			ImageItems[i]->SetBrushTintColor(ActivatedColor);
			PageSwitcher->SetActiveWidgetIndex(Index);
		}else
		{
			ImageItems[i]->SetBrushTintColor(NotActivatedColor);
		}
	}
}

void UTalesInventoryMainLeftUserWidget::InitializeData()
{
	EquipShieldSlot = nullptr;
	EquipSwardSlot = nullptr;
	if(TalesCharacterOwner && ensureAlways(SlotClass))
	{
		FTalesInventoryPackageDatas Datas = TalesCharacterOwner->GetTalesInventoryComponent()->GetPackagesDatas();	
		// Sward
		InitializeOnePageData(Datas.Sward, SwardWrapBox, TalesCharacterOwner->GetTalesInventoryComponent()->GetSwardSlot(), this->EquipSwardSlot);
		InitializeOnePageData(Datas.Shield, ShieldWrapBox, TalesCharacterOwner->GetTalesInventoryComponent()->GetShieldSlot(), this->EquipShieldSlot);
		InitializeOnePageData_Eatable(Datas.Eatable, EatableWrapBox);
	}
}

void UTalesInventoryMainLeftUserWidget::InitializeOnePageData(TMultiMap<FName, FTalesInventoryItemSlot>& PageData,
	UWrapBox*& Box, FTalesInventoryItemSlot ActivateItem, UTalesSlotUserWidget*& EquipSlot)
{
	if(Box)
	{
		Box->ClearChildren();
		if(PageData.IsEmpty())
		{
			// 添加默认显示框
			UTalesSlotUserWidget* Widget = CreateWidget<UTalesSlotUserWidget>(this, SlotClass);
			Box->AddChildToWrapBox(Widget);
		}else
		{
			for(auto& item : PageData)
			{
				UTalesSlotUserWidget* Widget = CreateWidget<UTalesSlotUserWidget>(this, SlotClass);
				Widget->SetDataConstruct(item.Value);
				Box->AddChildToWrapBox(Widget);
				if(ActivateItem.IsValid() && item.Value == ActivateItem && EquipSlot == nullptr)
				{
					EquipSlot = Widget;
					Widget->ActivateSlot();
				}
			}
		}
	}
}

void UTalesInventoryMainLeftUserWidget::InitializeOnePageData_Eatable(TMultiMap<FName, FTalesInventoryItemSlot>& PageData,
	UWrapBox*& Box)
{
	if(Box)
	{
		Box->ClearChildren();
		if(PageData.IsEmpty())
		{
			// 添加默认显示框
			UTalesSlotUserWidget* Widget = CreateWidget<UTalesSlotUserWidget>(this, SlotClass);
			Box->AddChildToWrapBox(Widget);
		}else
		{
			for(auto& item : PageData)
			{
				// 判断一下
				int StackMaxNum = item.Value.Quantity;
				auto Row = item.Value.GetRow();
				if(Row)
				{
					StackMaxNum = Row->StackSize;	
				}
				if(item.Value.Quantity > StackMaxNum)
				{
					UTalesSlotUserWidget* Widget = CreateWidget<UTalesSlotUserWidget>(this, SlotClass);
					auto BoxData = item.Value;
					BoxData.Quantity = StackMaxNum;
					Widget->SetDataConstruct(BoxData);
					Box->AddChildToWrapBox(Widget);

					item.Value.Quantity -= StackMaxNum;
				}
				UTalesSlotUserWidget* Widget = CreateWidget<UTalesSlotUserWidget>(this, SlotClass);
				Widget->SetDataConstruct(item.Value);
				Box->AddChildToWrapBox(Widget);
			}
		}
	}
}

void UTalesInventoryMainLeftUserWidget::SetTalesCharacterOwner()
{
	auto Character = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	TalesCharacterOwner = Cast<ATalesCharacter>(Character);
	if(!IsValid(TalesCharacterOwner))
	{
		TalesCharacterOwner = nullptr;
		UE_LOG(LogTemp, Error, TEXT("Inventroy Main Left: Error to Get TalesCharacter."))
	}
}

void UTalesInventoryMainLeftUserWidget::SetSwardSlotOnActivate(UTalesSlotUserWidget* NewSwardSlot)
{
	// 确保不删除空指针
	if(IsValid(this->EquipSwardSlot))
	{
		this->EquipSwardSlot->UnActivateSlot();
	}
	this->EquipSwardSlot = NewSwardSlot;
	this->EquipSwardSlot->ActivateSlot();
}

void UTalesInventoryMainLeftUserWidget::SetShieldSlotOnActivate(UTalesSlotUserWidget* NewShieldSlot)
{
	if(IsValid(this->EquipShieldSlot))
	{
		this->EquipShieldSlot->UnActivateSlot();
	}
	this->EquipShieldSlot = NewShieldSlot;
	this->EquipShieldSlot->ActivateSlot();
}
