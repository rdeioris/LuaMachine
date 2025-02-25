// Copyright 2018-2023 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LuaState.h"
#include "LuaValue.h"
#include "LuaTableAsset.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class LUAMACHINE_API ULuaTableAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditAnywhere, Category = "Lua")
	TMap<FString, FLuaValue> Table;

	FLuaValue ToLuaTable(ULuaState* LuaState);
};
