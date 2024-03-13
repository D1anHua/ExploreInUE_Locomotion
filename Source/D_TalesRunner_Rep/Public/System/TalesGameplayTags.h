﻿#pragma once

#include "NativeGameplayTags.h"

namespace TalesGameplayTags
{
	D_TALESRUNNER_REP_API FGameplayTag FindTagByString(const FString& TagString, bool bMatchPartialString = false);

	// Declare all of the custom native tags that Tales will use
	D_TALESRUNNER_REP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Move);
	D_TALESRUNNER_REP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Sprint);
	D_TALESRUNNER_REP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Jump);
	D_TALESRUNNER_REP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Utimate);

	D_TALESRUNNER_REP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InitState_Spawned);
	D_TALESRUNNER_REP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InitState_DataAvailable);
	D_TALESRUNNER_REP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InitState_DataInitialized);
	D_TALESRUNNER_REP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InitState_GameplayReady);

	D_TALESRUNNER_REP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Mode_Walking);
	D_TALESRUNNER_REP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Mode_NavWalking);
	D_TALESRUNNER_REP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Mode_Falling);
	D_TALESRUNNER_REP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Mode_Swimming);
	D_TALESRUNNER_REP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Mode_Flying);
	
	D_TALESRUNNER_REP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Mode_Custom);

	D_TALESRUNNER_REP_API extern const TMap<uint8, FGameplayTag> MovementModeTagMap;
	D_TALESRUNNER_REP_API extern const TMap<uint8, FGameplayTag> CustomMovementModeTagMap;

	D_TALESRUNNER_REP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_MovementStopped);
}