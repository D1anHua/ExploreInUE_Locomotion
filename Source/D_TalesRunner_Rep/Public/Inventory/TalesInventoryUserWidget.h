// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TalesInventoryUserWidget.generated.h"

class UTalesHealthBarUserWidget;
/**
 * 
 */
UCLASS()
class D_TALESRUNNER_REP_API UTalesInventoryUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	bool Initialize() override;
	
	UFUNCTION()
	FORCEINLINE class UTalesInfoCueUserWidget* GetInfoCueUserWidget() const { return WBPItemsInfoCue; }
	UTalesHealthBarUserWidget* GetHealthBarUserWidget() const { return WBPHealthBar; }


	UFUNCTION()
	FORCEINLINE UTalesInventoryMainLeftUserWidget* GetMainLeftMenu() const { return WBPMainLeft; }

protected:
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* MoneyAmountText;

	UPROPERTY(meta = (BindWidget))
	class UTalesHealthBarUserWidget* WBPHealthBar;
	UPROPERTY(meta = (BindWidget))
	class UTalesInfoCueUserWidget* WBPItemsInfoCue;

	UPROPERTY(meta = (BindWidget), BlueprintReadWrite)
	class UTalesInventoryMainLeftUserWidget* WBPMainLeft;
	// UFUNCTION()
	// void MoneyAmountUpdate(AActor* InstigateActor, UTalesInventoryComponent* OwnComp, int32 MoneyAmount, int32 delta);
};
