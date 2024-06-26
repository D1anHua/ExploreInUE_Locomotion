// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/TalesInventoryUserWidget.h"
#include "Character/TalesCharacter.h"
#include "Components/TextBlock.h"
#include "Inventory/TalesInventoryComponent.h"
#include "Inventory/UserWidget/TalesInfoCueUserWidget.h"
#include "Kismet/GameplayStatics.h"

void UTalesInventoryUserWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// auto OwnerPawn = GetOwningPlayer();
	auto OwnerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	ATalesCharacter* TalesCharacter = Cast<ATalesCharacter>(OwnerPawn);
	if(TalesCharacter)
	{
		if(MoneyAmountText)
		{
			auto InitialMoneyAmount = TalesCharacter->GetTalesInventoryComponent()->GetMoneyAmount();
			MoneyAmountText->SetText(FText::FromString(FString::FromInt(InitialMoneyAmount)));
		}
		// TalesCharacter->GetTalesInventoryComponent()->MoneyAmountChangeDelegate.AddDynamic(this, &ThisClass::MoneyAmountUpdate);
	}
	if(WBPItemsInfoCue)
	{
		WBPItemsInfoCue->SetVisibility(ESlateVisibility::Hidden);
	}
}

bool UTalesInventoryUserWidget::Initialize()
{
	bool Success = Super::Initialize();
	if(!Success) return false;
	if(MoneyAmountText)
	{
		// auto OwnerPawn = GetOwningPlayer();
		auto OwnerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
		// auto OwnerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
		ATalesCharacter* TalesCharacter = Cast<ATalesCharacter>(OwnerPawn);
		if(TalesCharacter)
		{
			auto InitialMoneyAmount = TalesCharacter->GetTalesInventoryComponent()->GetMoneyAmount();
			MoneyAmountText->SetText(FText::FromString(FString::FromInt(InitialMoneyAmount)));
		}
		else
		{
			MoneyAmountText->SetText(FText::FromString("0"));
		}	
	}
	return true;
}
