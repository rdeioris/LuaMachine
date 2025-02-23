// Copyright 2018-2023 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "ThirdParty/LuauLibrary/VM/include/lua.h"
#include "ThirdParty/LuauLibrary/VM/include/lualib.h"
#include "LuauValue.h"
#include "LuauCode.h"
#include "Runtime/Core/Public/Containers/Queue.h"
#include "Runtime/Launch/Resources/Version.h"
#include "LuauDelegate.h"
#include "LuauCommandExecutor.h"
#include "LuauState.generated.h"

LUAUMACHINE_API DECLARE_LOG_CATEGORY_EXTERN(LogLuauMachine, Log, All);

/**
 *
 */

class ULuauBlueprintPackage;

struct FLuauUserData
{
	ELuauValueType Type;
	// we use weak pointers as both fields can eventually be garbage collected
	// while the lua VM hold a reference to the userdata
	TWeakObjectPtr<UObject> Context;
	TWeakObjectPtr<UFunction> Function;

	// meaningful only for multicast delegates broadcasting
	FMulticastScriptDelegate* MulticastScriptDelegate;

	FLuauUserData(UObject* InObject)
	{
		Type = ELuauValueType::UObject;
		Context = InObject;
		MulticastScriptDelegate = nullptr;
	}

	FLuauUserData(UObject* InObject, UFunction* InFunction)
	{
		Type = ELuauValueType::UFunction;
		Context = InObject;
		Function = InFunction;
		MulticastScriptDelegate = nullptr;
	}
};

UENUM(BlueprintType)
enum class ELuauThreadStatus : uint8
{
	Invalid,
	Ok,
	Suspended,
	Error,
};

USTRUCT()
struct FLuauLibsLoader
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Luau", meta = (DisplayName = "Load base"))
	bool bLoadBase;

	UPROPERTY(EditAnywhere, Category = "Luau", meta = (DisplayName = "Load coroutine"))
	bool bLoadCoroutine;

	UPROPERTY(EditAnywhere, Category = "Luau", meta = (DisplayName = "Load table"))
	bool bLoadTable;

	UPROPERTY(EditAnywhere, Category = "Luau", meta = (DisplayName = "Load io"))
	bool bLoadIO;

	UPROPERTY(EditAnywhere, Category = "Luau", meta = (DisplayName = "Load os"))
	bool bLoadOS;

	UPROPERTY(EditAnywhere, Category = "Luau", meta = (DisplayName = "Load string"))
	bool bLoadString;

	UPROPERTY(EditAnywhere, Category = "Luau", meta = (DisplayName = "Load math"))
	bool bLoadMath;

	UPROPERTY(EditAnywhere, Category = "Luau", meta = (DisplayName = "Load utf8"))
	bool bLoadUTF8;

	UPROPERTY(EditAnywhere, Category = "Luau", meta = (DisplayName = "Load debug"))
	bool bLoadDebug;

	FLuauLibsLoader()
		: bLoadBase(true)
		, bLoadCoroutine(true)
		, bLoadTable(true)
		, bLoadIO(true)
		, bLoadOS(true)
		, bLoadString(true)
		, bLoadMath(true)
		, bLoadUTF8(true)
		, bLoadDebug(false)
	{}

};

USTRUCT(BlueprintType)
struct FLuauDebug
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Luau")
	int32 CurrentLine;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Luau")
	FString Source;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Luau")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Luau")
	FString NameWhat;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Luau")
	FString What;

	FLuauDebug()
		: CurrentLine(0)
	{

	}
};

USTRUCT(BlueprintType)
struct FLuauDelegateGroup
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<ULuauDelegate*> LuauDelegates;
};


struct FLuauSmartReference : public TSharedFromThis<FLuauSmartReference>
{
	ULuauState* LuauState;
	FLuauValue Value;
};


class ULuauUserDataObject;

UCLASS(Abstract, Blueprintable, HideDropdown)
class LUAUMACHINE_API ULuauState : public UObject
{
	GENERATED_BODY()

public:
	ULuauState();
	~ULuauState();

	virtual UWorld* GetWorld() const override { return CurrentWorld; }

	UPROPERTY(EditAnywhere, Category = "Luau")
	ULuauCode* LuauCodeAsset;

	UPROPERTY(EditAnywhere, Category = "Luau")
	FString LuauFilename;

	UPROPERTY(EditAnywhere, Category = "Luau")
	TMap<FString, FLuauValue> Table;

	UPROPERTY(EditAnywhere, Category = "Luau", meta = (DisplayName = "Luau Blueprint Packages Table"))
	TMap<FString, TSubclassOf<ULuauBlueprintPackage>> LuauBlueprintPackagesTable;

	UPROPERTY(EditAnywhere, Category = "Luau")
	TMap<FString, ULuauCode*> RequireTable;

	UPROPERTY(EditAnywhere, Category = "Luau")
	bool bLuauOpenLibs;

	UPROPERTY(EditAnywhere, Category = "Luau", meta = (DisplayName = "Load Specific Luau Libraries (only if \"Luau Open Libs\" is false)"))
	FLuauLibsLoader LuauLibsLoader;

	UPROPERTY(EditAnywhere, Category = "Luau")
	bool bAddProjectContentDirToPackagePath;

	UPROPERTY(EditAnywhere, Category = "Luau")
	TArray<FString> AppendProjectContentDirSubDir;

	UPROPERTY(EditAnywhere, Category = "Luau")
	FString OverridePackagePath;

	UPROPERTY(EditAnywhere, Category = "Luau")
	FString OverridePackageCPath;

	UPROPERTY(EditAnywhere, meta = (DisplayName = "UserData MetaTable from CodeAsset"), Category = "Luau")
	ULuauCode* UserDataMetaTableFromCodeAsset;

	UFUNCTION(BlueprintNativeEvent, Category = "Luau", meta = (DisplayName = "Luau Error"))
	void ReceiveLuauError(const FString& Message);

	UFUNCTION(BlueprintNativeEvent, Category = "Luau", meta = (DisplayName = "Luau Line Hook"))
	void ReceiveLuauLineHook(const FLuauDebug& LuauDebug);

	UFUNCTION(BlueprintNativeEvent, Category = "Luau", meta = (DisplayName = "Luau Call Hook"))
	void ReceiveLuauCallHook(const FLuauDebug& LuauDebug);

	UFUNCTION(BlueprintNativeEvent, Category = "Luau", meta = (DisplayName = "Luau Return Hook"))
	void ReceiveLuauReturnHook(const FLuauDebug& LuauDebug);

	// Not BlueprintNativeEvent, as throwing a luaL_error from an RTTI call results in leaving the VM in an unexpected
	// state and will result in exceptions
	UFUNCTION(Category = "Luau", meta = (DisplayName = "Luau Count Hook"))
	virtual void ReceiveLuauCountHook(const FLuauDebug& LuauDebug);

	UFUNCTION(BlueprintCallable, Category = "Luau")
	FLuauValue NewLuauUserDataObject(TSubclassOf<ULuauUserDataObject> LuauUserDataObjectClass, bool bTrackObject=true);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Luau")
	FLuauDebug LuauGetInfo(const int32 Level);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Luau")
	TMap<FString, FLuauValue> LuauGetLocals(const int32 Level);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Luau")
	TSubclassOf<ULuauState> GetSelfLuauState() const { return GetClass(); }

	template<class T>
	FLuauValue NewLuauUserDataObject(bool bTrackObject = true)
	{
		return NewLuauUserDataObject(T::StaticClass(), bTrackObject);
	}

	UFUNCTION(BlueprintCallable, Category = "Luau")
	void SetLuauUserDataField(FLuauValue UserData, const FString& Key, FLuauValue Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Luau")
	FLuauValue GetLuauUserDataField(FLuauValue UserData, const FString& Key);

	UFUNCTION(BlueprintCallable, Category = "Luau")
	FLuauValue GetLuauValueFromProperty(UObject* InObject, const FString& PropertyName);

	UFUNCTION(BlueprintCallable, Category = "Luau")
	bool SetPropertyFromLuauValue(UObject* InObject, const FString& PropertyName, FLuauValue Value);

	UFUNCTION(BlueprintCallable, BlueprintPure,  Category = "Luau")
	FLuauValue GetLuauBlueprintPackageTable(const FString& PackageName);

	void FromLuauValue(FLuauValue& LuauValue, UObject* CallContext = nullptr, lua_State* State = nullptr);
	FLuauValue ToLuauValue(int Index, lua_State* State = nullptr);

	ELuauThreadStatus GetLuauThreadStatus(FLuauValue Value);
	int32 GetLuauThreadStackTop(FLuauValue Value);

	UPROPERTY(EditAnywhere, Category = "Luau")
	bool bLogError;

	/* Enable it if you want this Luau state to not be destroyed during PIE. Useful for editor scripting */
	UPROPERTY(EditAnywhere, Category = "Luau")
	bool bPersistent;

	/* Enable debug of each Luau line. The LuauLineHook event will be triggered */
	UPROPERTY(EditAnywhere, Category = "Luau")
	bool bEnableLineHook;

	/* Enable debug of each Luau call. The LuauCallHook event will be triggered */
	UPROPERTY(EditAnywhere, Category = "Luau")
	bool bEnableCallHook;

	/* Enable debug of each Luau return. The LuauReturnHook event will be triggered */
	UPROPERTY(EditAnywhere, Category = "Luau")
	bool bEnableReturnHook;

	/* Enable debug for reaching a number of Luau instruction. The LuauCountHook event will be triggered */
	UPROPERTY(EditAnywhere, Category = "Luau")
	bool bEnableCountHook;

	/* Number of instructions to wait for when the Count Hook is enabled */
	UPROPERTY(EditAnywhere, Category = "Luau", Meta = (EditCondition = "bEnableCountHook"))
	int32 HookInstructionCount = 25000;

	UPROPERTY()
	TMap<FString, ULuauBlueprintPackage*> LuauBlueprintPackages;

	TArray<TSharedRef<FLuauSmartReference>> LuauSmartReferences;

	UPROPERTY()
	TArray<ULuauUserDataObject*> TrackedLuauUserDataObjects;

	UFUNCTION(BlueprintNativeEvent, Category = "Luau", meta = (DisplayName = "Luau Level Added To World"))
	void ReceiveLuauLevelAddedToWorld(ULevel* Level, UWorld* World);

	UFUNCTION(BlueprintNativeEvent, Category = "Luau", meta = (DisplayName = "Luau Level Removed From World"))
	void ReceiveLuauLevelRemovedFromWorld(ULevel* Level, UWorld* World);

	UFUNCTION(BlueprintNativeEvent, Category = "Luau", meta = (DisplayName = "Luau State Pre Initialization"))
	void ReceiveLuauStatePreInitialized();
	
	UFUNCTION(BlueprintNativeEvent, Category = "Luau", meta = (DisplayName = "Luau State Initialized"))
	void ReceiveLuauStateInitialized();

	int32 GetTop();

	FString LastError;

	int32 InceptionLevel;

	TQueue<FString> InceptionErrors;

	void NewTable();

	void SetMetaTable(int Index);
	void GetMetaTable(int Index);

	void SetField(int Index, const char* FieldName);

	void GetField(int Index, const char* FieldName);

	void NewUObject(UObject* Object, lua_State* State);

	void* NewUserData(size_t DataSize);

	void GetGlobal(const char* Name);

	int32 GetFieldFromTree(const FString& Tree, bool bGlobal = true);

	void SetFieldFromTree(const FString& Tree, FLuauValue& Value, bool bGlobal, UObject* CallContext = nullptr);

	void SetGlobal(const char* Name);

	void PushValue(int Index);

	void PushGlobalTable();

	bool PCall(int NArgs, FLuauValue& Value, int NRet = 1);
	bool Call(int NArgs, FLuauValue& Value, int NRet = 1);

	void Pop(int32 Amount = 1);

	void PushNil();

	void Unref(int Ref);
	void UnrefChecked(int Ref);
	int NewRef();
	void GetRef(int Ref);
	int Next(int Index);

	bool Resume(int Index, int NArgs);
	bool Yield(int Index, int NArgs);

	int GC(int What, int Data = 0);

	int32 ToInteger(int Index);

	void Len(int Index);

	int32 ILen(int Index);

	void RawGetI(int Index, int N);
	void RawSetI(int Index, int N);

	void PushCFunction(lua_CFunction Function);

	ULuauState* GetLuauState(UWorld* InWorld);

	bool RunCode(const TArray<uint8>& Code, const FString& CodePath, int NRet = 0);
	bool RunCode(const FString& Code, const FString& CodePath, int NRet = 0);

	bool RunCodeAsset(ULuauCode* CodeAsset, int NRet = 0);

	FLuauValue CreateLuauTable();
	FLuauValue CreateLuauThread(FLuauValue Value);

	FLuauValue CreateLuauLazyTable();

	bool RunFile(const FString& Filename, bool bIgnoreNonExistent, int NRet = 0, bool bNonContentDirectory=false);

	static int MetaTableFunctionUserData__index(lua_State* L);
	static int MetaTableFunctionUserData__newindex(lua_State* L);

	static int TableFunction_print(lua_State* L);
	static int TableFunction_package_preload(lua_State* L);
	static int TableFunction_package_loader(lua_State* L);
	static int TableFunction_package_loader_codeasset(lua_State* L);
	static int TableFunction_package_loader_asset(lua_State* L);

	static int MetaTableFunction__call(lua_State* L);
	static int MetaTableFunction__rawcall(lua_State* L);
	static int MetaTableFunction__rawbroadcast(lua_State* L);

	static int MetaTableFunctionUserData__eq(lua_State* L);
	static int MetaTableFunctionUserData__gc(lua_State* L);

	static int ToByteCode_Writer(lua_State* L, const void* Ptr, size_t Size, void* UserData);

	static void Debug_Hook(lua_State* L, lua_Debug* ar);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Luau")
	static TArray<uint8> ToByteCode(const FString& Code, const FString& CodePath, FString& ErrorString);

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	FLuauValue FromUProperty(void* Buffer, FProperty* Property, bool& bSuccess, int32 Index = 0);
	void ToUProperty(void* Buffer, FProperty* Property, FLuauValue Value, bool& bSuccess, int32 Index = 0);
	FLuauValue FromFProperty(void* Buffer, FProperty* Property, bool& bSuccess, int32 Index = 0);
	void ToFProperty(void* Buffer, FProperty* Property, FLuauValue Value, bool& bSuccess, int32 Index = 0);
	FLuauValue FromProperty(void* Buffer, FProperty* Property, bool& bSuccess, int32 Index = 0);
	void ToProperty(void* Buffer, FProperty* Property, FLuauValue Value, bool& bSuccess, int32 Index = 0);
#else
	FLuauValue FromUProperty(void* Buffer, UProperty* Property, bool& bSuccess, int32 Index = 0);
	void ToUProperty(void* Buffer, UProperty* Property, FLuauValue Value, bool& bSuccess, int32 Index = 0);
	FLuauValue FromProperty(void* Buffer, UProperty* Property, bool& bSuccess, int32 Index = 0);
	void ToProperty(void* Buffer, UProperty* Property, FLuauValue Value, bool& bSuccess, int32 Index = 0);
#endif

	static ULuauState* GetFromExtraSpace(lua_State* L)
	{
		ULuauState** LuauExtraSpacePtr = (ULuauState**)lua_getthreaddata(L);
		return *LuauExtraSpacePtr;
	}

	void Log(const FString& Message)
	{
		UE_LOG(LogLuauMachine, Log, TEXT("%s"), *Message);
	}

	void LogWarning(const FString& Message)
	{
		UE_LOG(LogLuauMachine, Warning, TEXT("%s"), *Message);
	}

	void LogError(const FString& Message)
	{
		UE_LOG(LogLuauMachine, Error, TEXT("%s"), *Message);
	}

	void SetUserDataMetaTable(FLuauValue MetaTable);

	FORCEINLINE lua_State* GetInternalLuauState() const { return L; }

	void PushRegistryTable();

	TSharedRef<FLuauSmartReference> AddLuauSmartReference(FLuauValue Value);
	void RemoveLuauSmartReference(TSharedRef<FLuauSmartReference> Ref);

	void SetupAndAssignUserDataMetatable(UObject* Context, TMap<FString, FLuauValue>& Metatable, lua_State* State);

	const void* ToPointer(int Index);

	UPROPERTY(EditAnywhere, Category = "Luau")
	bool bRawLuauFunctionCall;

	void GCLuauDelegatesCheck();

	void RegisterLuauDelegate(UObject* InObject, ULuauDelegate* InLuauDelegate);
	void UnregisterLuauDelegatesOfObject(UObject* InObject);

	TArray<FString> GetPropertiesNames(UObject* InObject);
	TArray<FString> GetFunctionsNames(UObject* InObject);

	FLuauValue StructToLuauTable(UScriptStruct* InScriptStruct, const uint8* StructData);

	UFUNCTION(BlueprintCallable, Category = "Luau")
	FLuauValue StructToLuauTable(UScriptStruct* InScriptStruct, const TArray<uint8>& StructData);

	void LuauTableToStruct(FLuauValue& LuauValue, UScriptStruct* InScriptStruct, uint8* StructData);

	template<class T>
	FLuauValue StructToLuauValue(T& InStruct)
	{
		return StructToLuauTable(T::StaticStruct(), (const uint8*)&InStruct);
	}

	template<class T>
	T LuauValueToStruct(FLuauValue& LuauValue)
	{
		T InStruct;
		LuauTableToStruct(LuauValue, T::StaticStruct(), (uint8*)&InStruct);
		return InStruct;
	}

	template<class T>
	T LuauValueToBaseStruct(FLuauValue& LuauValue)
	{
		T InStruct;
		LuauTableToStruct(LuauValue, TBaseStructure<T>::Get(), (uint8*)&InStruct);
		return InStruct;
	}

	FORCEINLINE FLuauCommandExecutor* GetLuauConsole() { return &LuauConsole; }

	UFUNCTION(BlueprintCallable, Category = "Luau")
	void AddLuauValueToLuauState(const FString& Name, FLuauValue LuauValue);

	UFUNCTION(BlueprintCallable, Category = "Luau")
	FLuauValue RunString(const FString& CodeString, FString CodePath);

	UFUNCTION(BlueprintCallable, Category = "Luau")
	void Error(const FString& ErrorString);

protected:
	lua_State* L;
	bool bDisabled;

	UWorld* CurrentWorld;

	FLuauValue UserDataMetaTable;

	virtual void LuauStateInit();

	FDelegateHandle GCLuauDelegatesHandle;

	UPROPERTY()
	TMap<TWeakObjectPtr<UObject>, FLuauDelegateGroup> LuauDelegatesMap;

	FLuauCommandExecutor LuauConsole;
};

#define LUACFUNCTION(FuncClass, FuncName, NumRetValues, NumArgs) static int FuncName ## _C(lua_State* L)\
{\
	FuncClass* LuauState = (FuncClass*)ULuauState::GetFromExtraSpace(L);\
	int TrueNumArgs = lua_gettop(L);\
	if (TrueNumArgs != NumArgs)\
	{\
		return luaL_error(L, "invalid number of arguments for %s (got %d, expected %d)", #FuncName, TrueNumArgs, NumArgs);\
	}\
	TArray<FLuauValue> LuauArgs;\
	for (int32 LuauArgIndex = 0; LuauArgIndex < NumArgs; LuauArgIndex++)\
	{\
		LuauArgs.Add(LuauState->ToLuauValue(LuauArgIndex + 1, L));\
	}\
	FLuauValue NilValue;\
	TArray<FLuauValue> RetValues = LuauState->FuncName(LuauArgs);\
	for (int32 RetIndex = 0; RetIndex < NumRetValues; RetIndex++)\
	{\
		if (RetIndex < RetValues.Num())\
		{\
			LuauState->FromLuauValue(RetValues[RetIndex]);\
		}\
		else\
		{\
			LuauState->FromLuauValue(NilValue);\
		}\
	}\
	return NumRetValues;\
}\
TArray<FLuauValue> FuncName(TArray<FLuauValue> LuauArgs)