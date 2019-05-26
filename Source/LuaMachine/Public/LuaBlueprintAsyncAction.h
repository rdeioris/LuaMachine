// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "LuaValue.h"
#include "LuaCode.h"
#include "LuaBlueprintAsyncAction.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLuaBlueprintAsyncResult, const TArray<FLuaValue>&, ReturnValues, FString, ErrorMessage);

/**
 *
 */
UCLASS()
class LUAMACHINE_API ULuaBlueprintAsyncAction : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FLuaBlueprintAsyncResult OnLuaAsyncResult;

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", AutoCreateRefTerm = "Args"), Category = "Lua")
	static ULuaBlueprintAsyncAction* LuaAsyncRunFromString(UObject* WorldContextObject, FString CodeString, TArray<FLuaValue> Args, bool bLuaOpenLibs = true);

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", AutoCreateRefTerm = "Args"), Category = "Lua")
	static ULuaBlueprintAsyncAction* LuaAsyncRunFromCodeAsset(UObject* WorldContextObject, ULuaCode* CodeAsset, TArray<FLuaValue> Args, bool bLuaOpenLibs = true);

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", AutoCreateRefTerm = "Args"), Category = "Lua")
	static ULuaBlueprintAsyncAction* LuaAsyncRunFromFile(UObject* WorldContextObject, FString Filename, TArray<FLuaValue> Args, bool bLuaOpenLibs = true);

	virtual void Activate() override;

protected:
	FString CodeString;
	FString CodePath;
	bool bLuaOpenLibs;
	TArray<FLuaValue> Args;
};
