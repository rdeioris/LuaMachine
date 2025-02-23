// Copyright 2018-2023 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LuauState.h"
#include "LuauValue.h"
#include "LuauTableAsset.h"
#include "UObject/TextProperty.h"
#include "Runtime/Engine/Classes/Engine/World.h"
#include "Runtime/Online/HTTP/Public/HttpModule.h"
#include "Sound/SoundWave.h"
#include "LuauBlueprintFunctionLibrary.generated.h"

/**
 * 
 */

DECLARE_DYNAMIC_DELEGATE_ThreeParams(FLuauHttpSuccess, FLuauValue, ReturnValue, bool, bWasSuccessful, int32, StatusCode);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FLuauHttpResponseReceived, FLuauValue, Context, FLuauValue, Response);
DECLARE_DYNAMIC_DELEGATE_OneParam(FLuauHttpError, FLuauValue, Context);

UENUM(BlueprintType)
enum class ELuauReflectionType : uint8
{
	Unknown,
	Property,
	Function,
};


UCLASS()
class LUAUMACHINE_API ULuauBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Luau")
	static FLuauValue LuauCreateNil();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Luau")
	static FLuauValue LuauCreateString(const FString& String);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Luau")
	static FLuauValue LuauCreateNumber(const float Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Luau")
	static FLuauValue LuauCreateInteger(const int32 Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Luau")
	static FLuauValue LuauCreateInteger64(const int64 Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Luau")
	static FLuauValue LuauCreateBool(const bool bInBool);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category="Luau")
	static FLuauValue LuauCreateTable(UObject* WorldContextObject, TSubclassOf<ULuauState> State);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "Luau")
	static FLuauValue LuauCreateThread(UObject* WorldContextObject, TSubclassOf<ULuauState> State, FLuauValue Value /* Function */);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Luau")
	static FLuauValue LuauCreateObject(UObject* InObject);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category="Luau")
	static FLuauValue LuauCreateObjectInState(UObject* WorldContextObject, TSubclassOf<ULuauState> State, UObject* InObject);

	UFUNCTION(BlueprintCallable, meta=(WorldContext="WorldContextObject"), Category="Luau")
	static FLuauValue LuauGetGlobal(UObject* WorldContextObject, TSubclassOf<ULuauState> State, const FString& Name);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category="Luau")
	static void LuauSetGlobal(UObject* WorldContextObject, TSubclassOf<ULuauState> State, const FString& Name, FLuauValue Value);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category="Luau")
	static void LuauSetUserDataMetaTable(UObject* WorldContextObject, TSubclassOf<ULuauState> State, FLuauValue MetaTable);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category="Luau")
	static FLuauValue AssignLuauValueToLuauState(UObject* WorldContextObject, FLuauValue Value, TSubclassOf<ULuauState> State);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Luau")
	static FLuauValue LuauTableGetField(FLuauValue Table, const FString& Key);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Luau")
	static FLuauValue GetLuauComponentAsLuauValue(AActor* Actor);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Luau")
	static FLuauValue GetLuauComponentByStateAsLuauValue(AActor* Actor, TSubclassOf<ULuauState> State);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Luau")
	static FLuauValue GetLuauComponentByNameAsLuauValue(AActor* Actor, const FString& Name);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Luau")
	static FLuauValue GetLuauComponentByStateAndNameAsLuauValue(AActor* Actor, TSubclassOf<ULuauState> State, const FString& Name);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Luau")
	static FLuauValue LuauComponentGetField(FLuauValue LuauComponent, const FString& Key);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Luau")
	static FLuauValue LuauTableGetByIndex(FLuauValue Table, const int32 Index);

	/* Assigns a value to a table index, returned value is the table itself */
	UFUNCTION(BlueprintCallable, Category="Luau")
	static FLuauValue LuauTableSetByIndex(FLuauValue Table, const int32 Index, FLuauValue Value);

	/* Returns the array of keys in the table */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Luau")
	static TArray<FLuauValue> LuauTableGetKeys(FLuauValue Table);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Luau")
	static TArray<FLuauValue> LuauTableGetValues(FLuauValue Table);

	/* Assigns a value to a table key, returned value is the table itself */
	UFUNCTION(BlueprintCallable, Category="Luau")
	static FLuauValue LuauTableSetField(FLuauValue Table, const FString& Key, FLuauValue Value);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "Args"), Category="Luau")
	static FLuauValue LuauGlobalCall(UObject* WorldContextObject, TSubclassOf<ULuauState> State, const FString& Name, TArray<FLuauValue> Args);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "Args"), Category="Luau")
	static TArray<FLuauValue> LuauGlobalCallMulti(UObject* WorldContextObject, TSubclassOf<ULuauState> State, const FString& Name, TArray<FLuauValue> Args);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "Args"), Category="Luau")
	static FLuauValue LuauGlobalCallValue(UObject* WorldContextObject, TSubclassOf<ULuauState> State, FLuauValue Value, TArray<FLuauValue> Args);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "Args"), Category="Luau")
	static TArray<FLuauValue> LuauGlobalCallValueMulti(UObject* WorldContextObject, TSubclassOf<ULuauState> State, FLuauValue Value, TArray<FLuauValue> Args);

	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "Args"), Category = "Luau")
	static ULuauState* LuauGetState(UObject* WorldContextObject, TSubclassOf<ULuauState> State);

	/* Calls a lua value (must be callable) */
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Args"), Category="Luau")
	static FLuauValue LuauValueCall(FLuauValue Value, TArray<FLuauValue> Args);

	/* Calls a lua value (must be callable and not nil) */
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Args"), Category="Luau")
	static FLuauValue LuauValueCallIfNotNil(FLuauValue Value, TArray<FLuauValue> Args);

	/* Calls a lua value taken from a table by key (must be callable) */
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Args"), Category="Luau")
	static FLuauValue LuauTableKeyCall(FLuauValue InTable, const FString& Key, TArray<FLuauValue> Args);

	/* Calls a lua value taken from a table by key (must be callable), passing the table itself as the first argument (useful for table:function syntax) */
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Args"), Category = "Luau")
	static FLuauValue LuauTableKeyCallWithSelf(FLuauValue InTable, const FString& Key, TArray<FLuauValue> Args);

	/* Calls a lua value taken from a table by index (must be callable) */
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Args"), Category="Luau")
	static FLuauValue LuauTableIndexCall(FLuauValue InTable, const int32 Index, TArray<FLuauValue> Args);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Luau")
	static TArray<FLuauValue> LuauTableUnpack(FLuauValue InTable);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "Luau")
	static FLuauValue LuauTablePack(UObject* WorldContextObject, TSubclassOf<ULuauState> State, TArray<FLuauValue> Values);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "Luau")
	static FLuauValue LuauTableMergePack(UObject* WorldContextObject, TSubclassOf<ULuauState> State, TArray<FLuauValue> Values1, TArray<FLuauValue> Values2);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Luau")
	static TArray<FLuauValue> LuauTableMergeUnpack(FLuauValue InTable1, FLuauValue InTable2);

	UFUNCTION(BlueprintCallable, Category = "Luau")
	static void LuauTableFillObject(FLuauValue InTable, UObject* InObject);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "Luau")
	static FLuauValue LuauTableFromMap(UObject* WorldContextObject, TSubclassOf<ULuauState> State, TMap<FString, FLuauValue> Map);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Luau")
	static TArray<FLuauValue> LuauTableRange(FLuauValue InTable, const int32 First, const int32 Last);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Luau")
	static TArray<FLuauValue> LuauValueArrayMerge(TArray<FLuauValue> Array1, TArray<FLuauValue> Array2);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Luau")
	static TArray<FLuauValue> LuauValueArrayAppend(TArray<FLuauValue> Array, FLuauValue Value);

	/* Calls a lua value with multiple return values (must be callable) */
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Args"), Category="Luau")
	static TArray<FLuauValue> LuauValueCallMulti(FLuauValue Value, TArray<FLuauValue> Args);

	/* Resume a lua coroutine/thread with multiple return values (must be callable) */
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Args"), Category = "Luau")
	static TArray<FLuauValue> LuauValueResumeMulti(FLuauValue Value, TArray<FLuauValue> Args);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Luau")
	static ELuauThreadStatus LuauThreadGetStatus(FLuauValue Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Luau")
	static int32 LuauThreadGetStackTop(FLuauValue Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Luau")
	static int32 LuauValueLength(FLuauValue Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Luau")
	static bool LuauValueIsReferencedInLuauRegistry(FLuauValue Value);

	UFUNCTION(BlueprintCallable, Category = "Luau")
	static UClass* LuauValueToBlueprintGeneratedClass(const FLuauValue& Value);

	UFUNCTION(BlueprintCallable, Category = "Luau")
	static UObject* LuauValueLoadObject(const FLuauValue& Value);

	UFUNCTION(BlueprintCallable, Category = "Luau")
	static UClass* LuauValueLoadClass(const FLuauValue& Value, const bool bDetectBlueprintGeneratedClass);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "Luau")
	static bool LuauValueFromJson(UObject* WorldContextObject, TSubclassOf<ULuauState> State, const FString& Json, FLuauValue& Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Luau")
	static FString LuauValueToJson(FLuauValue Value);

	UFUNCTION(BlueprintCallable, Category = "Luau")
	static FLuauValue LuauValueFromBase64(const FString& Base64);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Luau")
	static FString LuauValueToBase64(const FLuauValue& Value);

	UFUNCTION(BlueprintCallable, Category = "Luau")
	static FLuauValue LuauValueFromUTF16(const FString& String);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Luau")
	static FString LuauValueToUTF16(const FLuauValue& Value);

	UFUNCTION(BlueprintCallable, Category = "Luau")
	static FLuauValue LuauValueFromUTF8(const FString& String);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Luau")
	static FString LuauValueToUTF8(const FLuauValue& Value);

	UFUNCTION(BlueprintCallable, Category = "Luau")
	static FLuauValue LuauValueFromUTF32(const FString& String);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Luau")
	static FString LuauValueToUTF32(const FLuauValue& Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (WorldContext = "WorldContextObject"), Category = "Luau")
	static int64 LuauValueToPointer(UObject* WorldContextObject, TSubclassOf<ULuauState> State, FLuauValue Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (WorldContext = "WorldContextObject"), Category = "Luau")
	static FString LuauValueToHexPointer(UObject* WorldContextObject, TSubclassOf<ULuauState> State, FLuauValue Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (WorldContext = "WorldContextObject"), Category="Luau")
	static int32 LuauGetTop(UObject* WorldContextObject, TSubclassOf<ULuauState> State);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "Luau")
	static void LuauStateDestroy(UObject* WorldContextObject, TSubclassOf<ULuauState> State);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "Luau")
	static void LuauStateReload(UObject* WorldContextObject, TSubclassOf<ULuauState> State);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category="Luau")
	static FLuauValue LuauRunFile(UObject* WorldContextObject, TSubclassOf<ULuauState> State, const FString& Filename, const bool bIgnoreNonExistent);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "Luau")
	static FLuauValue LuauRunNonContentFile(UObject* WorldContextObject, TSubclassOf<ULuauState> State, const FString& Filename, const bool bIgnoreNonExistent);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category="Luau")
	static FLuauValue LuauRunCodeAsset(UObject* WorldContextObject, TSubclassOf<ULuauState> State, ULuauCode* CodeAsset);
	
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category="Luau")
	static FLuauValue LuauRunByteCode(UObject* WorldContextObject, TSubclassOf<ULuauState> State, const TArray<uint8>& ByteCode, const FString& CodePath);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category="Luau")
	static FLuauValue LuauRunString(UObject* WorldContextObject, TSubclassOf<ULuauState> State, const FString& CodeString, FString CodePath="");

	/* Make an HTTP GET request to the specified URL to download the Luau script to run */
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "Headers"), Category = "Luau")
	static void LuauRunURL(UObject* WorldContextObject, TSubclassOf<ULuauState> State, const FString& URL, TMap<FString, FString> Headers, const FString& SecurityHeader, const FString& SignaturePublicExponent, const FString& SignatureModulus, FLuauHttpSuccess Completed);

	/* Make an HTTP GET request to the specified URL to download the Luau script to run */
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "Headers,Error,ResponseReceived"), Category = "Luau")
	static void LuauHttpRequest(UObject* WorldContextObject, TSubclassOf<ULuauState> State, const FString& Method, const FString& URL, TMap<FString, FString> Headers, FLuauValue Body, FLuauValue Context, const FLuauHttpResponseReceived& ResponseReceived, const FLuauHttpError& Error);

	UFUNCTION(BlueprintCallable, Category = "Luau")
	static UTexture2D* LuauValueToTransientTexture(const int32 Width, const int32 Height, const FLuauValue& Value, const EPixelFormat PixelFormat = EPixelFormat::PF_B8G8R8A8, bool bDetectFormat = false);


	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Args"), Category = "Luau")
	static void LuauValueYield(FLuauValue Value, TArray<FLuauValue> Args);

	UFUNCTION(BlueprintCallable, Category = "Luau")
	static bool LuauLoadPakFile(const FString& Filename, FString Mountpoint, TArray<FLuauValue>& Assets, FString ContentPath, FString AssetRegistryPath);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "Luau")
	static FLuauValue LuauNewLuauUserDataObject(UObject* WorldContextObject, TSubclassOf<ULuauState> State, TSubclassOf<ULuauUserDataObject> UserDataObjectClass, bool bTrackObject=true);

	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (WorldContext = "WorldContextObject"), Category="Luau")
	static int32 LuauGetUsedMemory(UObject* WorldContextObject, TSubclassOf<ULuauState> State);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category="Luau")
	static void LuauGCCollect(UObject* WorldContextObject, TSubclassOf<ULuauState> State);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category="Luau")
	static void LuauGCStop(UObject* WorldContextObject, TSubclassOf<ULuauState> State);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category="Luau")
	static void LuauGCRestart(UObject* WorldContextObject, TSubclassOf<ULuauState> State);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "Luau")
	static FLuauValue LuauTableAssetToLuauTable(UObject* WorldContextObject, TSubclassOf<ULuauState> State, ULuauTableAsset* TableAsset);

	UFUNCTION(BlueprintCallable, Category = "Luau")
	static bool LuauTableImplements(FLuauValue Table, ULuauTableAsset* TableAsset);

	UFUNCTION(BlueprintCallable, Category = "Luau")
	static bool LuauTableImplementsAll(FLuauValue Table, TArray<ULuauTableAsset*> TableAssets);

	UFUNCTION(BlueprintCallable, Category = "Luau")
	static bool LuauTableImplementsAny(FLuauValue Table, TArray<ULuauTableAsset*> TableAssets);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Luau")
	static bool LuauValueIsNil(const FLuauValue& Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Luau")
	static bool LuauValueIsOwned(const FLuauValue& Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Luau")
	static TSubclassOf<ULuauState> LuauValueGetOwner(const FLuauValue& Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Luau")
	static bool LuauValueIsNotNil(const FLuauValue& Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Luau")
	static bool LuauValueIsTable(const FLuauValue& Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Luau")
	static bool LuauValueIsBoolean(const FLuauValue& Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Luau")
	static bool LuauValueIsNumber(const FLuauValue& Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Luau")
	static bool LuauValueIsInteger(const FLuauValue& Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Luau")
	static bool LuauValueIsString(const FLuauValue& Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Luau")
	static bool LuauValueIsFunction(const FLuauValue& Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Luau")
	static bool LuauValueIsThread(const FLuauValue& Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Luau")
	static FVector LuauTableToVector(FLuauValue Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Luau")
	static FRotator LuauTableToRotator(FLuauValue Value);

	UFUNCTION(BlueprintCallable, Category = "Luau")
	static FLuauValue LuauTableSetMetaTable(FLuauValue InTable, FLuauValue InMetaTable);

	UFUNCTION(BlueprintPure, meta=(DisplayName = "To String (LuauValue)", BlueprintAutocast), Category="Luau")
	static FString Conv_LuauValueToString(const FLuauValue& Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Text (LuauValue)", BlueprintAutocast), Category="Luau")
	static FText Conv_LuauValueToText(const FLuauValue& Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Name (LuauValue)", BlueprintAutocast), Category="Luau")
	static FName Conv_LuauValueToName(const FLuauValue& Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Object (LuauValue)", BlueprintAutocast), Category="Luau")
	static UObject* Conv_LuauValueToObject(const FLuauValue& Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Class (LuauValue)", BlueprintAutocast), Category="Luau")
	static UClass* Conv_LuauValueToClass(const FLuauValue& Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To LuauValue (Object)", BlueprintAutocast), Category="Luau")
	static FLuauValue Conv_ObjectToLuauValue(UObject* Object);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To LuauValue (Float)", BlueprintAutocast), Category="Luau")
	static FLuauValue Conv_FloatToLuauValue(const float Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Int (LuauValue)", BlueprintAutocast), Category="Luau")
	static int32 Conv_LuauValueToInt(const FLuauValue& Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Int64 (LuauValue)", BlueprintAutocast), Category = "Luau")
	static int64 Conv_LuauValueToInt64(const FLuauValue& Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Float (LuauValue)", BlueprintAutocast), Category="Luau")
	static float Conv_LuauValueToFloat(const FLuauValue& Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Vector (LuauValue)", BlueprintAutocast), Category = "Luau")
	static FVector Conv_LuauValueToFVector(const FLuauValue& Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Rotator (LuauValue)", BlueprintAutocast), Category = "Luau")
	static FRotator Conv_LuauValueToFRotator(const FLuauValue& Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Bool (LuauValue)", BlueprintAutocast), Category="Luau")
	static bool Conv_LuauValueToBool(const FLuauValue& Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To LuauValue (Int)", BlueprintAutocast), Category="Luau")
	static FLuauValue Conv_IntToLuauValue(const int32 Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To LuauValue (Int64)", BlueprintAutocast), Category = "Luau")
	static FLuauValue Conv_Int64ToLuauValue(const int64 Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To LuauValue (String)", BlueprintAutocast), Category="Luau")
	static FLuauValue Conv_StringToLuauValue(const FString& Value);

	UFUNCTION(BlueprintPure, meta = (BDisplayName = "To LuauValue (Text)", lueprintAutocast), Category="Luau")
	static FLuauValue Conv_TextToLuauValue(const FText& Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To LuauValue (Name)", BlueprintAutocast), Category="Luau")
	static FLuauValue Conv_NameToLuauValue(const FName Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To LuauValue (Bool)", BlueprintAutocast), Category="Luau")
	static FLuauValue Conv_BoolToLuauValue(const bool Value);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "Luau")
	static FLuauValue LuauCreateLazyTable(UObject* WorldContextObject, TSubclassOf<ULuauState> State);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Luau")
	static FLuauValue LuauCreateUFunction(UObject* InObject, const FString& FunctionName);

	UFUNCTION(BlueprintCallable, meta = (ExpandEnumAsExecs = "LuauValueTypes"), Category = "Luau")
	static void SwitchOnLuauValueType(const FLuauValue& LuauValue, ELuauValueType& LuauValueTypes);

	UFUNCTION(BlueprintCallable, meta = (ExpandEnumAsExecs = "LuauReflectionTypes"), Category = "Luau")
	static void GetLuauReflectionType(UObject* InObject, const FString& Name, ELuauReflectionType& LuauReflectionTypes);

	UFUNCTION(BlueprintCallable, Category = "Luau")
	static void RegisterLuauConsoleCommand(const FString& CommandName, const FLuauValue& LuauConsoleCommand);

	UFUNCTION(BlueprintCallable, Category = "Luau")
	static void UnregisterLuauConsoleCommand(const FString& CommandName);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "Luau")
	static ULuauState* CreateDynamicLuauState(UObject* WorldContextObject, TSubclassOf<ULuauState> LuauStateClass);

private:
	static void HttpRequestDone(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, TSubclassOf<ULuauState> LuauState, TWeakObjectPtr<UWorld> World, const FString SecurityHeader, const FString SignaturePublicExponent, const FString SignatureModulus, FLuauHttpSuccess Completed);
	static void HttpGenericRequestDone(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, TWeakPtr<FLuauSmartReference> Context, FLuauHttpResponseReceived ResponseReceived, FLuauHttpError Error);

};
