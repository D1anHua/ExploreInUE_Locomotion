// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TalesRunnerTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TalesBlueprintLibrary.generated.h"

/**
 * 
 */
UCLASS()
class D_TALESRUNNER_REP_API UTalesBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static const UItemStaticData* GetItemStaticData(TSubclassOf<UItemStaticData> ItemDataClass);
};
