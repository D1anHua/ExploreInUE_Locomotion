#include "System/TalesRunnerTypes.h"

bool UItemStaticData::CanEquipped() const
{
	switch (ItemType)
	{
	case SwardItem:
	case ShieldItem:
		return true;
		break;
	default:
		break;
	}
	return false;
}

bool UItemStaticData::CanUsed() const
{
	switch (ItemType)
	{
	case EatableItem:
		return true;
		break;
	default:
		break;
	}
	return false;
	
}

FName UItemStaticData::GetAttackSocket() const
{
	switch (ItemType)
	{
	case SwardItem:
		return FName("SwardSocket");
		break;
	case ShieldItem:
		return FName("ShieldSocket");
		break;
	default:
		break;
	}
	return FName("Null");
}
