// Copyright 2018-2025 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "LuaVMIncludes.h"
#include "LuaValue.h"
#include "LuaCode.h"
#include "Runtime/Core/Public/Containers/Queue.h"
#include "Runtime/Launch/Resources/Version.h"
#include "LuaDelegate.h"
#include "LuaCommandExecutor.h"
#include "LuaState.generated.h"

LUAMACHINE_API DECLARE_LOG_CATEGORY_EXTERN(LogLuaMachine, Log, All);

/**
 *
 */

class ULuaBlueprintPackage;

// NOTE: this struct holds non-trivial members (TSharedPtr), so every instance
// living in lua-owned memory MUST be created with placement new and destroyed
// with an explicit destructor call from a __gc metamethod. Assigning the fields
// of a raw lua_newuserdata() block leaves the non-trivial members uninitialized.
struct FLuaUserData
{
	ELuaValueType Type = ELuaValueType::Nil;
	// we use weak pointers as both fields can eventually be garbage collected
	// while the lua VM hold a reference to the userdata
	TWeakObjectPtr<UObject> Context;
	TWeakObjectPtr<UFunction> Function;

	// meaningful only for multicast delegates broadcasting
	FMulticastScriptDelegate* MulticastScriptDelegate = nullptr;

	TSharedPtr<TFunction<FLuaValueOrError(TArray<FLuaValue>)>> Lambda;

	FLuaUserData(UObject* InObject)
	{
		Type = ELuaValueType::UObject;
		Context = InObject;
	}

	FLuaUserData(UObject* InObject, UFunction* InFunction)
	{
		Type = ELuaValueType::UFunction;
		Context = InObject;
		Function = InFunction;
	}

	FLuaUserData(UFunction* InFunction, FMulticastScriptDelegate* InMulticastScriptDelegate)
	{
		Type = ELuaValueType::MulticastDelegate;
		Function = InFunction;
		MulticastScriptDelegate = InMulticastScriptDelegate;
	}

	FLuaUserData(TSharedPtr<TFunction<FLuaValueOrError(TArray<FLuaValue>)>> InLambda)
	{
		Type = ELuaValueType::Lambda;
		Lambda = InLambda;
	}
};

UENUM(BlueprintType)
enum class ELuaThreadStatus : uint8
{
	Invalid,
	Ok,
	Suspended,
	Error,
};

USTRUCT()
struct FLuaLibsLoader
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Lua", meta = (DisplayName = "Load base"))
	bool bLoadBase;

	UPROPERTY(EditAnywhere, Category = "Lua", meta = (DisplayName = "Load coroutine"))
	bool bLoadCoroutine;

	UPROPERTY(EditAnywhere, Category = "Lua", meta = (DisplayName = "Load table"))
	bool bLoadTable;

	UPROPERTY(EditAnywhere, Category = "Lua", meta = (DisplayName = "Load io"))
	bool bLoadIO;

	UPROPERTY(EditAnywhere, Category = "Lua", meta = (DisplayName = "Load os"))
	bool bLoadOS;

	UPROPERTY(EditAnywhere, Category = "Lua", meta = (DisplayName = "Load string"))
	bool bLoadString;

	UPROPERTY(EditAnywhere, Category = "Lua", meta = (DisplayName = "Load math"))
	bool bLoadMath;

	UPROPERTY(EditAnywhere, Category = "Lua", meta = (DisplayName = "Load utf8"))
	bool bLoadUTF8;

	UPROPERTY(EditAnywhere, Category = "Lua", meta = (DisplayName = "Load debug"))
	bool bLoadDebug;

	FLuaLibsLoader()
		: bLoadBase(true)
		, bLoadCoroutine(true)
		, bLoadTable(true)
		, bLoadIO(true)
		, bLoadOS(true)
		, bLoadString(true)
		, bLoadMath(true)
		, bLoadUTF8(true)
		, bLoadDebug(false)
	{
	}

};

USTRUCT(BlueprintType)
struct FLuaDebug
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lua")
	int32 CurrentLine;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lua")
	FString Source;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lua")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lua")
	FString NameWhat;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lua")
	FString What;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lua")
	FString ShortSource;

	FLuaDebug()
		: CurrentLine(0)
	{

	}
};

USTRUCT(BlueprintType)
struct FLuaProfiledCall
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lua")
	FString Source;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lua")
	int32 Line = -1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lua")
	FString Call;

	bool operator==(const FLuaProfiledCall& Other) const
	{
		return Source == Other.Source && Line == Other.Line && Call == Other.Call;
	}
};

USTRUCT(BlueprintType)
struct FLuaProfiledStack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lua")
	TArray<FLuaProfiledCall> CallStack;

	bool operator==(const FLuaProfiledStack& Other) const
	{
		return CallStack == Other.CallStack;
	}
};

USTRUCT(BlueprintType)
struct FLuaProfiledData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lua")
	TArray<FLuaProfiledCall> CallStack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lua")
	double Duration = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lua")
	int64 Count = 0;
};

FORCEINLINE uint32 GetTypeHash(const FLuaProfiledStack& LuaProfiledStack)
{
	uint32 Hash = 0;

	for (const FLuaProfiledCall& Call : LuaProfiledStack.CallStack)
	{
		Hash = HashCombine(Hash, GetTypeHash(Call.Source));
		Hash = HashCombine(Hash, GetTypeHash(Call.Line));
		Hash = HashCombine(Hash, GetTypeHash(Call.Call));
	}

	return Hash;
}

USTRUCT(BlueprintType)
struct FLuaDelegateGroup
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<ULuaDelegate*> LuaDelegates;
};


struct FLuaSmartReference : public TSharedFromThis<FLuaSmartReference>
{
	ULuaState* LuaState;
	FLuaValue Value;
};


class ULuaUserDataObject;

UCLASS(Abstract, Blueprintable, HideDropdown)
class LUAMACHINE_API ULuaState : public UObject
{
	GENERATED_BODY()

public:
	ULuaState();
	~ULuaState();

	virtual UWorld* GetWorld() const override { return CurrentWorld; }

	UPROPERTY(EditAnywhere, Category = "Lua")
	ULuaCode* LuaCodeAsset;

	UPROPERTY(EditAnywhere, Category = "Lua")
	FString LuaFilename;

	UPROPERTY(EditAnywhere, Category = "Lua")
	TMap<FString, FLuaValue> Table;

	UPROPERTY(EditAnywhere, Category = "Lua", meta = (DisplayName = "Lua Blueprint Packages Table"))
	TMap<FString, TSubclassOf<ULuaBlueprintPackage>> LuaBlueprintPackagesTable;

	UPROPERTY(EditAnywhere, Category = "Lua")
	TMap<FString, ULuaCode*> RequireTable;

	UPROPERTY(EditAnywhere, Category = "Lua")
	bool bLuaOpenLibs;

	UPROPERTY(EditAnywhere, Category = "Lua", meta = (DisplayName = "Load Specific Lua Libraries (only if \"Lua Open Libs\" is false)"))
	FLuaLibsLoader LuaLibsLoader;

	UPROPERTY(EditAnywhere, Category = "Lua")
	bool bAddProjectContentDirToPackagePath;

	UPROPERTY(EditAnywhere, Category = "Lua")
	TArray<FString> AppendProjectContentDirSubDir;

	UPROPERTY(EditAnywhere, Category = "Lua")
	FString OverridePackagePath;

	UPROPERTY(EditAnywhere, Category = "Lua")
	FString OverridePackageCPath;

	UPROPERTY(EditAnywhere, meta = (DisplayName = "UserData MetaTable from CodeAsset"), Category = "Lua")
	ULuaCode* UserDataMetaTableFromCodeAsset;

	UFUNCTION(BlueprintNativeEvent, Category = "Lua", meta = (DisplayName = "Lua Error"))
	void ReceiveLuaError(const FString& Message);

	UFUNCTION(BlueprintNativeEvent, Category = "Lua", meta = (DisplayName = "Lua Line Hook"))
	void ReceiveLuaLineHook(const FLuaDebug& LuaDebug);

	UFUNCTION(BlueprintNativeEvent, Category = "Lua", meta = (DisplayName = "Lua Call Hook"))
	void ReceiveLuaCallHook(const FLuaDebug& LuaDebug);

	UFUNCTION(BlueprintNativeEvent, Category = "Lua", meta = (DisplayName = "Lua Return Hook"))
	void ReceiveLuaReturnHook(const FLuaDebug& LuaDebug);

	UFUNCTION(BlueprintNativeEvent, Category = "Lua", meta = (DisplayName = "Lua SingleStep Hook"))
	void ReceiveLuaSingleStepHook(const FLuaDebug& LuaDebug);

	// Not BlueprintNativeEvent, as throwing a luaL_error from an RTTI call results in leaving the VM in an unexpected
	// state and will result in exceptions
	UFUNCTION(Category = "Lua", meta = (DisplayName = "Lua Count Hook"))
	virtual void ReceiveLuaCountHook(const FLuaDebug& LuaDebug);

	UFUNCTION(BlueprintCallable, Category = "Lua")
	FLuaValue NewLuaUserDataObject(TSubclassOf<ULuaUserDataObject> LuaUserDataObjectClass, bool bTrackObject = true);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lua")
	FLuaDebug LuaGetInfo(const int32 Level);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lua")
	TMap<FString, FLuaValue> LuaGetLocals(const int32 Level);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lua")
	TSubclassOf<ULuaState> GetSelfLuaState() const { return GetClass(); }

	template<class T>
	FLuaValue NewLuaUserDataObject(bool bTrackObject = true)
	{
		return NewLuaUserDataObject(T::StaticClass(), bTrackObject);
	}

	UFUNCTION(BlueprintCallable, Category = "Lua")
	void SetLuaUserDataField(FLuaValue UserData, const FString& Key, FLuaValue Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lua")
	FLuaValue GetLuaUserDataField(FLuaValue UserData, const FString& Key);

	UFUNCTION(BlueprintCallable, Category = "Lua")
	FLuaValue GetLuaValueFromProperty(UObject* InObject, const FString& PropertyName);

	UFUNCTION(BlueprintCallable, Category = "Lua")
	bool SetPropertyFromLuaValue(UObject* InObject, const FString& PropertyName, FLuaValue Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lua")
	FLuaValue GetLuaBlueprintPackageTable(const FString& PackageName);

	void FromLuaValue(FLuaValue& LuaValue, UObject* CallContext = nullptr, lua_State* State = nullptr);
	FLuaValue ToLuaValue(int Index, lua_State* State = nullptr);

	ELuaThreadStatus GetLuaThreadStatus(FLuaValue Value);
	int32 GetLuaThreadStackTop(FLuaValue Value);

	UPROPERTY(EditAnywhere, Category = "Lua")
	bool bLogError;

	/* Enable it if you want this Lua state to not be destroyed during PIE. Useful for editor scripting */
	UPROPERTY(EditAnywhere, Category = "Lua")
	bool bPersistent;

	/* Enable debug of each Lua line. The LuaLineHook event will be triggered */
	UPROPERTY(EditAnywhere, Category = "Lua")
	bool bEnableLineHook;

	/* Enable debug of each Lua call. The LuaCallHook event will be triggered */
	UPROPERTY(EditAnywhere, Category = "Lua")
	bool bEnableCallHook;

	/* Enable debug of each Lua return. The LuaReturnHook event will be triggered */
	UPROPERTY(EditAnywhere, Category = "Lua")
	bool bEnableReturnHook;

	/* Enable debug for reaching a number of Lua instruction. The LuaCountHook event will be triggered */
	UPROPERTY(EditAnywhere, Category = "Lua")
	bool bEnableCountHook;

	/* Number of instructions to wait for when the Count Hook is enabled */
	UPROPERTY(EditAnywhere, Category = "Lua", Meta = (EditCondition = "bEnableCountHook"))
	int32 HookInstructionCount = 25000;

	UPROPERTY()
	TMap<FString, ULuaBlueprintPackage*> LuaBlueprintPackages;

	TArray<TSharedRef<FLuaSmartReference>> LuaSmartReferences;

	UPROPERTY()
	TArray<ULuaUserDataObject*> TrackedLuaUserDataObjects;

	UFUNCTION(BlueprintNativeEvent, Category = "Lua", meta = (DisplayName = "Lua Level Added To World"))
	void ReceiveLuaLevelAddedToWorld(ULevel* Level, UWorld* World);

	UFUNCTION(BlueprintNativeEvent, Category = "Lua", meta = (DisplayName = "Lua Level Removed From World"))
	void ReceiveLuaLevelRemovedFromWorld(ULevel* Level, UWorld* World);

	UFUNCTION(BlueprintNativeEvent, Category = "Lua", meta = (DisplayName = "Lua State Pre Initialization"))
	void ReceiveLuaStatePreInitialized();

	UFUNCTION(BlueprintNativeEvent, Category = "Lua", meta = (DisplayName = "Lua State Initialized"))
	void ReceiveLuaStateInitialized();

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

	void SetFieldFromTree(const FString& Tree, FLuaValue& Value, bool bGlobal, UObject* CallContext = nullptr);

	void SetGlobal(const char* Name);

	void PushValue(int Index);

	void PushGlobalTable();

	bool PCall(int NArgs, FLuaValue& Value, int NRet = 1);
	bool Call(int NArgs, FLuaValue& Value, int NRet = 1);

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

	ULuaState* GetLuaState(UWorld* InWorld);

	bool RunCode(const TArray<uint8>& Code, const FString& CodePath, int NRet = 0);
	bool RunCode(const FString& Code, const FString& CodePath, int NRet = 0);

	bool RunCodeAsset(ULuaCode* CodeAsset, int NRet = 0);

	FLuaValue CreateLuaTable();
	FLuaValue CreateLuaThread(FLuaValue Value);

	FLuaValue CreateLuaLazyTable();

	bool RunFile(const FString& Filename, bool bIgnoreNonExistent, int NRet = 0, bool bNonContentDirectory = false);

	static int MetaTableFunctionUserData__index(lua_State* L);
	static int MetaTableFunctionUserData__newindex(lua_State* L);

	static int MetaTableFunctionUserDataInterface__index(lua_State* L);
	static int MetaTableFunctionUserDataInterface__newindex(lua_State* L);
	static int MetaTableFunctionUserDataInterface__gc(lua_State* L);
	static int MetaTableFunctionUserDataInterface__tostring(lua_State* L);

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

	static int MetaTableFunctionLambda__gc(lua_State* L);

	static int ToByteCode_Writer(lua_State* L, const void* Ptr, size_t Size, void* UserData);

	static void OnAllocateCallback(lua_State* L, size_t OSize, size_t NSize);
	static void OnInterrupt(lua_State* L, int gc);

	static void OnProfile(lua_State* L, int gc);

	static void Debug_Hook(lua_State* L, lua_Debug* ar);

	static void Debug_SingleStep(lua_State* L, lua_Debug* ar);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lua")
	static TArray<uint8> ToByteCode(const FString& Code, const FString& CodePath, FString& ErrorString);

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	FLuaValue FromUProperty(void* Buffer, FProperty* Property, bool& bSuccess, int32 Index = 0);
	void ToUProperty(void* Buffer, FProperty* Property, FLuaValue Value, bool& bSuccess, int32 Index = 0);
	FLuaValue FromFProperty(void* Buffer, FProperty* Property, bool& bSuccess, int32 Index = 0);
	void ToFProperty(void* Buffer, FProperty* Property, FLuaValue Value, bool& bSuccess, int32 Index = 0);
	FLuaValue FromProperty(void* Buffer, FProperty* Property, bool& bSuccess, int32 Index = 0);
	void ToProperty(void* Buffer, FProperty* Property, FLuaValue Value, bool& bSuccess, int32 Index = 0);
#else
	FLuaValue FromUProperty(void* Buffer, UProperty* Property, bool& bSuccess, int32 Index = 0);
	void ToUProperty(void* Buffer, UProperty* Property, FLuaValue Value, bool& bSuccess, int32 Index = 0);
	FLuaValue FromProperty(void* Buffer, UProperty* Property, bool& bSuccess, int32 Index = 0);
	void ToProperty(void* Buffer, UProperty* Property, FLuaValue Value, bool& bSuccess, int32 Index = 0);
#endif

	static ULuaState* GetFromExtraSpace(lua_State* L)
	{
		ULuaState** LuaExtraSpacePtr = (ULuaState**)lua_getextraspace(L);
		return *LuaExtraSpacePtr;
	}

	void Log(const FString& Message)
	{
		UE_LOG(LogLuaMachine, Log, TEXT("%s"), *Message);
	}

	void LogWarning(const FString& Message)
	{
		UE_LOG(LogLuaMachine, Warning, TEXT("%s"), *Message);
	}

	void LogError(const FString& Message)
	{
		UE_LOG(LogLuaMachine, Error, TEXT("%s"), *Message);
	}

	void SetUserDataMetaTable(FLuaValue MetaTable);

	FORCEINLINE lua_State* GetInternalLuaState() const { return L; }

	void PushRegistryTable();

	TSharedRef<FLuaSmartReference> AddLuaSmartReference(FLuaValue Value);
	void RemoveLuaSmartReference(TSharedRef<FLuaSmartReference> Ref);

	void SetupAndAssignUserDataMetatable(UObject* Context, TMap<FString, FLuaValue>& Metatable, lua_State* State);
	void SetupAndAssignUserDataInterfaceMetatable(class ILuaUserDataInterface* LuaUserDataInterface, lua_State* State);

	const void* ToPointer(int Index);

	UPROPERTY(EditAnywhere, Category = "Lua")
	bool bRawLuaFunctionCall;

	void GCLuaDelegatesCheck();

	void RegisterLuaDelegate(UObject* InObject, ULuaDelegate* InLuaDelegate);
	void UnregisterLuaDelegatesOfObject(UObject* InObject);

	TArray<FString> GetPropertiesNames(UObject* InObject);
	TArray<FString> GetFunctionsNames(UObject* InObject);

	FLuaValue StructToLuaTable(UScriptStruct* InScriptStruct, const uint8* StructData);

	UFUNCTION(BlueprintCallable, Category = "Lua")
	FLuaValue StructToLuaTable(UScriptStruct* InScriptStruct, const TArray<uint8>& StructData);

	void LuaTableToStruct(FLuaValue& LuaValue, UScriptStruct* InScriptStruct, uint8* StructData);

	template<class T>
	FLuaValue StructToLuaValue(T& InStruct)
	{
		return StructToLuaTable(T::StaticStruct(), (const uint8*)&InStruct);
	}

	template<class T>
	T LuaValueToStruct(FLuaValue& LuaValue)
	{
		T InStruct;
		LuaTableToStruct(LuaValue, T::StaticStruct(), (uint8*)&InStruct);
		return InStruct;
	}

	template<class T>
	T LuaValueToBaseStruct(FLuaValue& LuaValue)
	{
		T InStruct;
		LuaTableToStruct(LuaValue, TBaseStructure<T>::Get(), (uint8*)&InStruct);
		return InStruct;
	}

	FORCEINLINE FLuaCommandExecutor* GetLuaConsole() { return &LuaConsole; }

	UFUNCTION(BlueprintCallable, Category = "Lua")
	void AddLuaValueToLuaState(const FString& Name, FLuaValue LuaValue);

	UFUNCTION(BlueprintCallable, Category = "Lua")
	FLuaValue RunString(const FString& CodeString, FString CodePath);

	UFUNCTION(BlueprintCallable, Category = "Lua")
	TArray<FLuaValue> RunStringMulti(const FString& CodeString, FString CodePath);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lua")
	FLuaValue GetLuaValueFromGlobalName(const FString& GlobalName);

	UFUNCTION(BlueprintCallable, Category = "Lua")
	void SetLuaValueFromGlobalName(const FString& Name, FLuaValue LuaValue);

	UFUNCTION(BlueprintCallable, Category = "Lua")
	FLuaValue LuaValueCall(FLuaValue LuaValue, TArray<FLuaValue> Args);

	UFUNCTION(BlueprintCallable, Category = "Lua")
	TArray<FLuaValue> LuaValueCallMulti(FLuaValue LuaValue, TArray<FLuaValue> Args);

	UFUNCTION(BlueprintCallable, Category = "Lua")
	void Error(const FString& ErrorString);

	template<typename T>
	static T* CreateDynamicLuaState(UWorld* InWorld)
	{
		T* NewLuaState = NewObject<T>((UObject*)GetTransientPackage());
		if (!NewLuaState)
		{
			return nullptr;
		}

		return Cast<T>(NewLuaState->GetLuaState(InWorld));
	}

	UPROPERTY(EditAnywhere, Category = "Lua")
	int64 MaxMemoryUsage = 0;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lua")
	int64 GetMemoryUsage() const { return CurrentMemoryUsage; }

	UFUNCTION(BlueprintCallable, Category = "Lua")
	void SetLuaTableReadonly(FLuaValue LuaValue, const bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Lua")
	void Sandbox();

	UFUNCTION(BlueprintCallable, Category = "Lua")
	FLuaValue RequireLuaBlueprintPackage(const FString& Name, TSubclassOf<ULuaBlueprintPackage> LuaBlueprintPackage);

	template<typename T>
	FLuaValue RequireLuaBlueprintPackage(const FString& Name)
	{
		return RequireLuaBlueprintPackage(Name, T::StaticClass());
	}

	UFUNCTION(BlueprintCallable, Category = "Lua")
	TArray<FLuaValue> LuaValueResume(FLuaValue LuaValue, TArray<FLuaValue> Args);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lua")
	int32 LuaValueLength(FLuaValue LuaValue);

	UFUNCTION(BlueprintCallable, Category = "Lua")
	void SetSingleStep(const bool bEnable);

	UFUNCTION(BlueprintCallable, Category = "Lua")
	void StartProfiler(const double Frequency = 0);

	UFUNCTION(BlueprintCallable, Category = "Lua")
	TMap<FLuaProfiledStack, FLuaProfiledData> StopProfiler();

	UPROPERTY(EditAnywhere, meta = (DisplayName = "Override Script Content Directory"), Category = "Lua")
	FString ScriptContentDirectory;

	UFUNCTION(BlueprintNativeEvent, Category = "Lua", meta = (DisplayName = "Override the default path where Lua scripts are searched for"))
	FString GetScriptContentDirectory() const;

	/* Start the Remote Debugger on the specified Host and port */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Start Remote Debugger"), Category = "Lua")
	bool StartRemoteDebugger(const FString& HostAndPort);

	/* Stop the Remote Debugger */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Stop Remote Debugger"), Category = "Lua")
	void StopRemoteDebugger();

	class FLuaRemoteDebugger* GetLuaRemoteDebugger() { return LuaRemoteDebugger; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category ="Lua")
	int32 GetStackDepth() const;

protected:
	lua_State* L;
	bool bDisabled;

	UWorld* CurrentWorld;

	FLuaValue UserDataMetaTable;

	FLuaValue DefaultUserDataMetaMethodEq;

	virtual void LuaStateInit();

	FDelegateHandle GCLuaDelegatesHandle;

	UPROPERTY()
	TMap<TWeakObjectPtr<UObject>, FLuaDelegateGroup> LuaDelegatesMap;

	FLuaCommandExecutor LuaConsole;

	int64 CurrentMemoryUsage;

	TMap<FLuaProfiledStack, FLuaProfiledData> CurrentProfiledStacks;

	void (*PreviousOnInterrupt)(lua_State* L, int gc) = nullptr;

	double ProfilerFrequency = 0;
	double LastProfilerRealTimeSeconds = 0;
	int64 ProfilerSamples = 0;

	FLuaRemoteDebugger* LuaRemoteDebugger = nullptr;
	FRunnableThread* LuaRemoteDebuggerThread;
	bool bRemoteDebuggerStarted = false;
};

#define LUACFUNCTION(FuncClass, FuncName, NumRetValues, NumArgs) static int FuncName ## _C(lua_State* L)\
{\
	FuncClass* LuaState = (FuncClass*)ULuaState::GetFromExtraSpace(L);\
	int TrueNumArgs = lua_gettop(L);\
	if (TrueNumArgs != NumArgs)\
	{\
		return luaL_error(L, "invalid number of arguments for %s (got %d, expected %d)", #FuncName, TrueNumArgs, NumArgs);\
	}\
	TArray<FLuaValue> LuaArgs;\
	for (int32 LuaArgIndex = 0; LuaArgIndex < NumArgs; LuaArgIndex++)\
	{\
		LuaArgs.Add(LuaState->ToLuaValue(LuaArgIndex + 1, L));\
	}\
	FLuaValue NilValue;\
	TArray<FLuaValue> RetValues = LuaState->FuncName(LuaArgs);\
	for (int32 RetIndex = 0; RetIndex < NumRetValues; RetIndex++)\
	{\
		if (RetIndex < RetValues.Num())\
		{\
			LuaState->FromLuaValue(RetValues[RetIndex]);\
		}\
		else\
		{\
			LuaState->FromLuaValue(NilValue);\
		}\
	}\
	return NumRetValues;\
}\
TArray<FLuaValue> FuncName(TArray<FLuaValue> LuaArgs)