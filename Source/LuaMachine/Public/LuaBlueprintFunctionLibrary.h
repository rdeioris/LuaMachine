// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LuaState.h"
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

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static FLuaValue LuaCreateNumber(float Value);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static FLuaValue LuaCreateInteger(int32 Value);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static FLuaValue LuaCreateBool(bool bInBool);

	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (WorldContext = "WorldContextObject"))
	static FLuaValue LuaCreateTable(UObject* WorldContextObject, TSubclassOf<ULuaState> State);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static FLuaValue LuaCreateObject(UObject* InObject);

	UFUNCTION(BlueprintCallable, meta=(WorldContext="WorldContextObject"))
	static FLuaValue LuaGetGlobal(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FString Name);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	static void LuaSetGlobal(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FString Name, FLuaValue Value);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static FLuaValue LuaTableGetField(FLuaValue Table, FString Name);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static FLuaValue LuaTableGetByIndex(FLuaValue Table, int32 Index);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static TArray<FLuaValue> LuaTableGetKeys(FLuaValue Table);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static TArray<FLuaValue> LuaTableGetValues(FLuaValue Table);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	static FLuaValue LuaTableSetField(UObject* WorldContextObject, FLuaValue Table, FString Key, FLuaValue Value);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "Args"))
	static FLuaValue LuaGlobalCall(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FString Name, TArray<FLuaValue> Args);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Args"))
	static FLuaValue LuaValueCall(FLuaValue Value, TArray<FLuaValue> Args);

	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (WorldContext = "WorldContextObject"))
	static int32 LuaGetTop(UObject* WorldContextObject, TSubclassOf<ULuaState> State);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	static FLuaValue LuaRunFile(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FString Filename, bool bIgnoreNonExistent);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	static FLuaValue LuaRunCodeAsset(UObject* WorldContextObject, TSubclassOf<ULuaState> State, ULuaCode* CodeAsset);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static bool LuaValueIsNil(FLuaValue Value);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static bool LuaValueIsTable(FLuaValue Value);

	UFUNCTION(BlueprintPure, meta=(BlueprintAutocast))
	static FString Conv_LuaValueToString(FLuaValue Value);

	UFUNCTION(BlueprintPure, meta = (BlueprintAutocast))
	static FText Conv_LuaValueToText(FLuaValue Value);

	UFUNCTION(BlueprintPure, meta = (BlueprintAutocast))
	static UObject* Conv_LuaValueToObject(FLuaValue Value);

	UFUNCTION(BlueprintPure, meta = (BlueprintAutocast))
	static UClass* Conv_LuaValueToClass(FLuaValue Value);

	UFUNCTION(BlueprintPure, meta = (BlueprintAutocast))
	static FLuaValue Conv_ObjectToLuaValue(UObject* Object);

	UFUNCTION(BlueprintPure, meta = (BlueprintAutocast))
	static int32 Conv_LuaValueToInt(FLuaValue Value);

	UFUNCTION(BlueprintPure, meta = (BlueprintAutocast))
	static float Conv_LuaValueToFloat(FLuaValue Value);

	UFUNCTION(BlueprintPure, meta = (BlueprintAutocast))
	static FLuaValue Conv_IntToLuaValue(int32 Value);

	UFUNCTION(BlueprintPure, meta = (BlueprintAutocast))
	static FLuaValue Conv_StringToLuaValue(FString Value);
	
};
