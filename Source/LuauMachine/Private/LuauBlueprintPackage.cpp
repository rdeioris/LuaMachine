// Copyright 2018-2023 - Roberto De Ioris
// Portions Copyright 2025 - Fusion Point Studios, Inc.


#include "LuauBlueprintPackage.h"
#include "LuauState.h"

TSubclassOf<ULuauState> ULuauBlueprintPackage::GetLuauState() const
{
	ULuauState* LuauState = Cast<ULuauState>(GetOuter());
	if (LuauState)
	{
		return LuauState->GetClass();
	}
	return nullptr;
}

ULuauState* ULuauBlueprintPackage::GetLuauStateInstance() const
{
	return Cast<ULuauState>(GetOuter());
}

UWorld* ULuauBlueprintPackage::GetWorld() const
{
	ULuauState* LuauState = Cast<ULuauState>(GetOuter());
	if (LuauState)
	{
		return LuauState->GetWorld();
	}
	return nullptr;
}

void ULuauBlueprintPackage::Init()
{

}