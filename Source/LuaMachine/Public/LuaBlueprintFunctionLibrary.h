// Copyright 2018 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LuaState.h"
#include "LuaValue.h"
#include "UObject/TextProperty.h"
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
	static FLuaValue LuaCreateNil();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static FLuaValue LuaCreateString(FString String);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static FLuaValue LuaCreateNumber(float Value);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static FLuaValue LuaCreateInteger(int32 Value);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static FLuaValue LuaCreateBool(bool bInBool);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	static FLuaValue LuaCreateTable(UObject* WorldContextObject, TSubclassOf<ULuaState> State);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static FLuaValue LuaCreateObject(UObject* InObject);

	UFUNCTION(BlueprintCallable, meta=(WorldContext="WorldContextObject"))
	static FLuaValue LuaGetGlobal(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FString Name);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	static void LuaSetGlobal(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FString Name, FLuaValue Value);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static FLuaValue LuaTableGetField(FLuaValue Table, FString Key);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static FLuaValue LuaComponentGetField(FLuaValue LuaComponent, FString Key);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static FLuaValue LuaTableGetByIndex(FLuaValue Table, int32 Index);

	/* Assigns a value to a table index, returned value is the table itself */
	UFUNCTION(BlueprintCallable)
	static FLuaValue LuaTableSetByIndex(FLuaValue Table, int32 Index, FLuaValue Value);

	/* Returns the array of keys in the table */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static TArray<FLuaValue> LuaTableGetKeys(FLuaValue Table);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static TArray<FLuaValue> LuaTableGetValues(FLuaValue Table);

	/* Assigns a value to a table key, returned value is the table itself */
	UFUNCTION(BlueprintCallable)
	static FLuaValue LuaTableSetField(FLuaValue Table, FString Key, FLuaValue Value);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "Args"))
	static FLuaValue LuaGlobalCall(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FString Name, TArray<FLuaValue> Args);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "Args"))
	static TArray<FLuaValue> LuaGlobalCallMulti(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FString Name, TArray<FLuaValue> Args);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "Args"))
	static FLuaValue LuaGlobalCallValue(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FLuaValue Value, TArray<FLuaValue> Args);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "Args"))
	static TArray<FLuaValue> LuaGlobalCallValueMulti(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FLuaValue Value, TArray<FLuaValue> Args);

	/* Calls a lua value (must be callable) */
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Args"))
	static FLuaValue LuaValueCall(FLuaValue Value, TArray<FLuaValue> Args);

	/* Calls a lua value (must be callable and not nil) */
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Args"))
	static FLuaValue LuaValueCallIfNotNil(FLuaValue Value, TArray<FLuaValue> Args);

	/* Calls a lua value taken from a table by key (must be callable) */
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Args"))
	static FLuaValue LuaTableKeyCall(FLuaValue InTable, FString Key, TArray<FLuaValue> Args);

	/* Calls a lua value taken from a table by index (must be callable) */
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Args"))
	static FLuaValue LuaTableIndexCall(FLuaValue InTable, int32 Index, TArray<FLuaValue> Args);

	/* Calls a lua value with multiple return values (must be callable) */
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Args"))
	static TArray<FLuaValue> LuaValueCallMulti(FLuaValue Value, TArray<FLuaValue> Args);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static int32 LuaValueLength(FLuaValue Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (WorldContext = "WorldContextObject"))
	static int32 LuaGetTop(UObject* WorldContextObject, TSubclassOf<ULuaState> State);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	static FLuaValue LuaRunFile(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FString Filename, bool bIgnoreNonExistent);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	static FLuaValue LuaRunCodeAsset(UObject* WorldContextObject, TSubclassOf<ULuaState> State, ULuaCode* CodeAsset);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	static FLuaValue LuaRunString(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FString CodeString);

	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (WorldContext = "WorldContextObject"))
	static int32 LuaGetUsedMemory(UObject* WorldContextObject, TSubclassOf<ULuaState> State);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	static void LuaGCCollect(UObject* WorldContextObject, TSubclassOf<ULuaState> State);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	static void LuaGCStop(UObject* WorldContextObject, TSubclassOf<ULuaState> State);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	static void LuaGCRestart(UObject* WorldContextObject, TSubclassOf<ULuaState> State);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static bool LuaValueIsNil(FLuaValue Value);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static bool LuaValueIsNotNil(FLuaValue Value);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static bool LuaValueIsTable(FLuaValue Value);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static bool LuaValueIsBoolean(FLuaValue Value);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static bool LuaValueIsNumber(FLuaValue Value);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static bool LuaValueIsInteger(FLuaValue Value);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static bool LuaValueIsString(FLuaValue Value);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static bool LuaValueIsFunction(FLuaValue Value);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static bool LuaValueIsThread(FLuaValue Value);

	UFUNCTION(BlueprintPure, meta=(DisplayName = "To String (LuaValue)", BlueprintAutocast))
	static FString Conv_LuaValueToString(FLuaValue Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Text (LuaValue)", BlueprintAutocast))
	static FText Conv_LuaValueToText(FLuaValue Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Name (LuaValue)", BlueprintAutocast))
	static FName Conv_LuaValueToName(FLuaValue Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Object (LuaValue)", BlueprintAutocast))
	static UObject* Conv_LuaValueToObject(FLuaValue Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Class (LuaValue)", BlueprintAutocast))
	static UClass* Conv_LuaValueToClass(FLuaValue Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To LuaValue (Object)", BlueprintAutocast))
	static FLuaValue Conv_ObjectToLuaValue(UObject* Object);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To LuaValue (Float)", BlueprintAutocast))
	static FLuaValue Conv_FloatToLuaValue(float Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Int (LuaValue)", BlueprintAutocast))
	static int32 Conv_LuaValueToInt(FLuaValue Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Float (LuaValue)", BlueprintAutocast))
	static float Conv_LuaValueToFloat(FLuaValue Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Bool (LuaValue)", BlueprintAutocast))
	static bool Conv_LuaValueToBool(FLuaValue Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To LuaValue (Int)", BlueprintAutocast))
	static FLuaValue Conv_IntToLuaValue(int32 Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To LuaValue (String)", BlueprintAutocast))
	static FLuaValue Conv_StringToLuaValue(FString Value);

	UFUNCTION(BlueprintPure, meta = (BDisplayName = "To LuaValue (Text)", lueprintAutocast))
	static FLuaValue Conv_TextToLuaValue(FText Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To LuaValue (Name)", BlueprintAutocast))
	static FLuaValue Conv_NameToLuaValue(FName Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To LuaValue (Bool)", BlueprintAutocast))
	static FLuaValue Conv_BoolToLuaValue(bool Value);
	
};
