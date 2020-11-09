// Copyright 2018-2020 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "LuaValue.h"
#include "LuaFunction.generated.h"

/**
 * 
 */
UCLASS()
class LUAMACHINE_API ULuaFunction : public UFunction
{
	GENERATED_BODY()

public:

	DECLARE_FUNCTION(CallLuaNative);

	UPROPERTY()
	ULuaState* LuaState;

	UPROPERTY()
	FLuaValue LuaFunction;
};
