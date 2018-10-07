// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LuaValue.h"
#include "LuaBlueprintFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class LUAMACHINE_API ULuaBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static FLuaValue LuaCreateString(FString String);

	UFUNCTION(BlueprintPure, meta=(BlueprintAutocast))
	static FString Conv_LuaValueToString(FLuaValue Value);
	
};
