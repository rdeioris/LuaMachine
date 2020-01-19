// Copyright 2019 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LuaState.h"
#include "LuaValue.h"
#include "LuaTableAsset.h"
#include "UObject/TextProperty.h"
#include "Runtime/Engine/Classes/Engine/World.h"
#include "Runtime/Online/HTTP/Public/HttpModule.h"
#include "LuaBlueprintFunctionLibrary.generated.h"

/**
 * 
 */

DECLARE_DYNAMIC_DELEGATE_ThreeParams(FLuaHttpSuccess, FLuaValue, ReturnValue, bool, bWasSuccessful, int32, StatusCode);


UCLASS()
class LUAMACHINE_API ULuaBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Lua")
	static FLuaValue LuaCreateNil();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Lua")
	static FLuaValue LuaCreateString(FString String);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Lua")
	static FLuaValue LuaCreateNumber(float Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Lua")
	static FLuaValue LuaCreateInteger(int32 Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Lua")
	static FLuaValue LuaCreateBool(bool bInBool);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category="Lua")
	static FLuaValue LuaCreateTable(UObject* WorldContextObject, TSubclassOf<ULuaState> State);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Lua")
	static FLuaValue LuaCreateObject(UObject* InObject);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category="Lua")
	static FLuaValue LuaCreateObjectInState(UObject* WorldContextObject, TSubclassOf<ULuaState> State, UObject* InObject);

	UFUNCTION(BlueprintCallable, meta=(WorldContext="WorldContextObject"), Category="Lua")
	static FLuaValue LuaGetGlobal(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FString Name);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category="Lua")
	static void LuaSetGlobal(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FString Name, FLuaValue Value);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category="Lua")
	static void LuaSetUserDataMetaTable(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FLuaValue MetaTable);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category="Lua")
	static FLuaValue AssignLuaValueToLuaState(UObject* WorldContextObject, FLuaValue Value, TSubclassOf<ULuaState> State);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Lua")
	static FLuaValue LuaTableGetField(FLuaValue Table, FString Key);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Lua")
	static FLuaValue GetLuaComponentAsLuaValue(AActor* Actor);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Lua")
	static FLuaValue GetLuaComponentByStateAsLuaValue(AActor* Actor, TSubclassOf<ULuaState> State);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Lua")
	static FLuaValue GetLuaComponentByNameAsLuaValue(AActor* Actor, FString Name);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Lua")
	static FLuaValue GetLuaComponentByStateAndNameAsLuaValue(AActor* Actor, TSubclassOf<ULuaState> State, FString Name);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Lua")
	static FLuaValue LuaComponentGetField(FLuaValue LuaComponent, FString Key);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Lua")
	static FLuaValue LuaTableGetByIndex(FLuaValue Table, int32 Index);

	/* Assigns a value to a table index, returned value is the table itself */
	UFUNCTION(BlueprintCallable, Category="Lua")
	static FLuaValue LuaTableSetByIndex(FLuaValue Table, int32 Index, FLuaValue Value);

	/* Returns the array of keys in the table */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Lua")
	static TArray<FLuaValue> LuaTableGetKeys(FLuaValue Table);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Lua")
	static TArray<FLuaValue> LuaTableGetValues(FLuaValue Table);

	/* Assigns a value to a table key, returned value is the table itself */
	UFUNCTION(BlueprintCallable, Category="Lua")
	static FLuaValue LuaTableSetField(FLuaValue Table, FString Key, FLuaValue Value);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "Args"), Category="Lua")
	static FLuaValue LuaGlobalCall(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FString Name, TArray<FLuaValue> Args);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "Args"), Category="Lua")
	static TArray<FLuaValue> LuaGlobalCallMulti(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FString Name, TArray<FLuaValue> Args);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "Args"), Category="Lua")
	static FLuaValue LuaGlobalCallValue(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FLuaValue Value, TArray<FLuaValue> Args);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "Args"), Category="Lua")
	static TArray<FLuaValue> LuaGlobalCallValueMulti(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FLuaValue Value, TArray<FLuaValue> Args);

	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "Args"), Category = "Lua")
	static ULuaState* LuaGetState(UObject* WorldContextObject, TSubclassOf<ULuaState> State);

	/* Calls a lua value (must be callable) */
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Args"), Category="Lua")
	static FLuaValue LuaValueCall(FLuaValue Value, TArray<FLuaValue> Args);

	/* Calls a lua value (must be callable and not nil) */
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Args"), Category="Lua")
	static FLuaValue LuaValueCallIfNotNil(FLuaValue Value, TArray<FLuaValue> Args);

	/* Calls a lua value taken from a table by key (must be callable) */
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Args"), Category="Lua")
	static FLuaValue LuaTableKeyCall(FLuaValue InTable, FString Key, TArray<FLuaValue> Args);

	/* Calls a lua value taken from a table by key (must be callable), passing the table itself as teh first argument (useful for table:function syntax) */
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Args"), Category = "Lua")
	static FLuaValue LuaTableKeyCallWithSelf(FLuaValue InTable, FString Key, TArray<FLuaValue> Args);

	/* Calls a lua value taken from a table by index (must be callable) */
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Args"), Category="Lua")
	static FLuaValue LuaTableIndexCall(FLuaValue InTable, int32 Index, TArray<FLuaValue> Args);

	/* Calls a lua value with multiple return values (must be callable) */
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Args"), Category="Lua")
	static TArray<FLuaValue> LuaValueCallMulti(FLuaValue Value, TArray<FLuaValue> Args);

	/* Resume a lua coroutine/thread with multiple return values (must be callable) */
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Args"), Category = "Lua")
	static TArray<FLuaValue> LuaValueResumeMulti(FLuaValue Value, TArray<FLuaValue> Args);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Lua")
	static int32 LuaValueLength(FLuaValue Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lua")
	static bool LuaValueIsReferencedInLuaRegistry(FLuaValue Value);

	UFUNCTION(BlueprintCallable, Category = "Lua")
	static UClass* LuaValueToBlueprintGeneratedClass(FLuaValue Value);

	UFUNCTION(BlueprintCallable, Category = "Lua")
	static UObject* LuaValueLoadObject(FLuaValue Value);

	UFUNCTION(BlueprintCallable, Category = "Lua")
	static UClass* LuaValueLoadClass(FLuaValue Value, bool bDetectBlueprintGeneratedClass);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "Lua")
	static bool LuaValueFromJson(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FString Json, FLuaValue& Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lua")
	static FString LuaValueToJson(FLuaValue Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (WorldContext = "WorldContextObject"), Category="Lua")
	static int32 LuaGetTop(UObject* WorldContextObject, TSubclassOf<ULuaState> State);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "Lua")
	static void LuaStateDestroy(UObject* WorldContextObject, TSubclassOf<ULuaState> State);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "Lua")
	static void LuaStateReload(UObject* WorldContextObject, TSubclassOf<ULuaState> State);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category="Lua")
	static FLuaValue LuaRunFile(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FString Filename, bool bIgnoreNonExistent);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category="Lua")
	static FLuaValue LuaRunCodeAsset(UObject* WorldContextObject, TSubclassOf<ULuaState> State, ULuaCode* CodeAsset);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category="Lua")
	static FLuaValue LuaRunString(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FString CodeString);

	/* Make an HTTP GET request to the specified URL to download the Lua script to run */
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "Headers"), Category = "Lua")
	static void LuaRunURL(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FString URL, TMap<FString, FString> Headers, FString SecurityHeader, FString SignaturePublicExponent, FString SignatureModulus, FLuaHttpSuccess Completed);

	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (WorldContext = "WorldContextObject"), Category="Lua")
	static int32 LuaGetUsedMemory(UObject* WorldContextObject, TSubclassOf<ULuaState> State);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category="Lua")
	static void LuaGCCollect(UObject* WorldContextObject, TSubclassOf<ULuaState> State);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category="Lua")
	static void LuaGCStop(UObject* WorldContextObject, TSubclassOf<ULuaState> State);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category="Lua")
	static void LuaGCRestart(UObject* WorldContextObject, TSubclassOf<ULuaState> State);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "Lua")
	static FLuaValue LuaTableAssetToLuaTable(UObject* WorldContextObject, TSubclassOf<ULuaState> State, ULuaTableAsset* TableAsset);

	UFUNCTION(BlueprintCallable, Category = "Lua")
	static bool LuaTableImplements(FLuaValue Table, ULuaTableAsset* TableAsset);

	UFUNCTION(BlueprintCallable, Category = "Lua")
	static bool LuaTableImplementsAll(FLuaValue Table, TArray<ULuaTableAsset*> TableAssets);

	UFUNCTION(BlueprintCallable, Category = "Lua")
	static bool LuaTableImplementsAny(FLuaValue Table, TArray<ULuaTableAsset*> TableAssets);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Lua")
	static bool LuaValueIsNil(FLuaValue Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Lua")
	static bool LuaValueIsOwned(FLuaValue Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Lua")
	static TSubclassOf<ULuaState> LuaValueGetOwner(FLuaValue Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Lua")
	static bool LuaValueIsNotNil(FLuaValue Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Lua")
	static bool LuaValueIsTable(FLuaValue Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Lua")
	static bool LuaValueIsBoolean(FLuaValue Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Lua")
	static bool LuaValueIsNumber(FLuaValue Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Lua")
	static bool LuaValueIsInteger(FLuaValue Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Lua")
	static bool LuaValueIsString(FLuaValue Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Lua")
	static bool LuaValueIsFunction(FLuaValue Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Lua")
	static bool LuaValueIsThread(FLuaValue Value);

	UFUNCTION(BlueprintPure, meta=(DisplayName = "To String (LuaValue)", BlueprintAutocast), Category="Lua")
	static FString Conv_LuaValueToString(FLuaValue Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Text (LuaValue)", BlueprintAutocast), Category="Lua")
	static FText Conv_LuaValueToText(FLuaValue Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Name (LuaValue)", BlueprintAutocast), Category="Lua")
	static FName Conv_LuaValueToName(FLuaValue Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Object (LuaValue)", BlueprintAutocast), Category="Lua")
	static UObject* Conv_LuaValueToObject(FLuaValue Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Class (LuaValue)", BlueprintAutocast), Category="Lua")
	static UClass* Conv_LuaValueToClass(FLuaValue Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To LuaValue (Object)", BlueprintAutocast), Category="Lua")
	static FLuaValue Conv_ObjectToLuaValue(UObject* Object);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To LuaValue (Float)", BlueprintAutocast), Category="Lua")
	static FLuaValue Conv_FloatToLuaValue(float Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Int (LuaValue)", BlueprintAutocast), Category="Lua")
	static int32 Conv_LuaValueToInt(FLuaValue Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Float (LuaValue)", BlueprintAutocast), Category="Lua")
	static float Conv_LuaValueToFloat(FLuaValue Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Bool (LuaValue)", BlueprintAutocast), Category="Lua")
	static bool Conv_LuaValueToBool(FLuaValue Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To LuaValue (Int)", BlueprintAutocast), Category="Lua")
	static FLuaValue Conv_IntToLuaValue(int32 Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To LuaValue (String)", BlueprintAutocast), Category="Lua")
	static FLuaValue Conv_StringToLuaValue(FString Value);

	UFUNCTION(BlueprintPure, meta = (BDisplayName = "To LuaValue (Text)", lueprintAutocast), Category="Lua")
	static FLuaValue Conv_TextToLuaValue(FText Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To LuaValue (Name)", BlueprintAutocast), Category="Lua")
	static FLuaValue Conv_NameToLuaValue(FName Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To LuaValue (Bool)", BlueprintAutocast), Category="Lua")
	static FLuaValue Conv_BoolToLuaValue(bool Value);

private:
	static void HttpRequestDone(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, TSubclassOf<ULuaState> LuaState, TWeakObjectPtr<UWorld> World, FString SecurityHeader, FString SignaturePublicExponent, FString SignatureModulus, FLuaHttpSuccess Completed);
	
};
