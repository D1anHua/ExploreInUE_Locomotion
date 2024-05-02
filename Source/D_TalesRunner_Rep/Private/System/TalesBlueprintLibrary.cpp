// Fill out your copyright notice in the Description page of Project Settings.


#include "System/TalesBlueprintLibrary.h"

const UItemStaticData* UTalesBlueprintLibrary::GetItemStaticData(TSubclassOf<UItemStaticData> ItemDataClass)
{
	if(IsValid(ItemDataClass))
	{
		return GetDefault<UItemStaticData>(ItemDataClass);	
	}
	return nullptr;
}
