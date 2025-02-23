// Copyright 2018-2023 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LuauState.h"
#include "LuauValue.h"
#include "LuauTableAsset.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class LUAUMACHINE_API ULuauTableAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditAnywhere, Category = "Luau")
	TMap<FString, FLuauValue> Table;

	FLuauValue ToLuauTable(ULuauState* LuauState);
};
