// Copyright 2018-2020 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "LuaClass.generated.h"

class ULuaState;

/**
 * 
 */
UCLASS()
class LUAMACHINE_API ULuaClass : public UClass
{
	GENERATED_BODY()

public:
	UPROPERTY()
	ULuaState* LuaState;

	UPROPERTY()
	FLuaValue LuaClassTable;
};
