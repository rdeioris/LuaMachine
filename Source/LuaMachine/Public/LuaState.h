// Copyright 2019 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "ThirdParty/lua/lua.hpp"
#include "LuaValue.h"
#include "LuaCode.h"
#include "Runtime/Core/Public/Containers/Queue.h"
#include "LuaState.generated.h"

LUAMACHINE_API DECLARE_LOG_CATEGORY_EXTERN(LogLuaMachine, Log, All);

/**
 *
 */

struct FLuaUserData
{
	ELuaValueType Type;
	// we use weak pointers as both fields can eventually be garbage collected
	// while the lua VM hold a reference to the userdata
	TWeakObjectPtr<UObject> Context;
	TWeakObjectPtr<UFunction> Function;

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
};


UCLASS(Abstract, Blueprintable, HideDropdown)
class LUAMACHINE_API ULuaState : public UObject
{
	GENERATED_BODY()

public:
	ULuaState();
	~ULuaState();

	virtual UWorld* GetWorld() const override { return CurrentWorld; }

	UPROPERTY(EditAnywhere, Category="Lua")
	ULuaCode* LuaCodeAsset;

	UPROPERTY(EditAnywhere, Category="Lua")
	FString LuaFilename;

	UPROPERTY(EditAnywhere, Category="Lua")
	TMap<FString, FLuaValue> Table;

	UPROPERTY(EditAnywhere, Category="Lua")
	TMap<FString, ULuaCode*> RequireTable;

	UPROPERTY(EditAnywhere, Category="Lua")
	bool bLuaOpenLibs;

	UPROPERTY(EditAnywhere, Category="Lua")
	bool bAddProjectContentDirToPackagePath;

	UPROPERTY(EditAnywhere, Category="Lua")
	TArray<FString> AppendProjectContentDirSubDir;

	UPROPERTY(EditAnywhere, Category="Lua")
	FString OverridePackagePath;

	UPROPERTY(EditAnywhere, Category="Lua")
	FString OverridePackageCPath;

	UPROPERTY(EditAnywhere, meta = (DisplayName = "UserData MetaTable from CodeAsset"), Category="Lua")
	ULuaCode* UserDataMetaTableFromCodeAsset;

	UFUNCTION(BlueprintImplementableEvent, Category = "Lua", meta = (DisplayName = "Lua Error"))
	void ReceiveLuaError(const FString& Message);

	void FromLuaValue(FLuaValue& LuaValue, UObject* CallContext = nullptr);
	FLuaValue ToLuaValue(int Index);

	UPROPERTY(EditAnywhere, Category="Lua")
	bool bLogError;

	/* Enable it if you want this Lua state to not be destroyed during PIE. Useful for editor scripting */
	UPROPERTY(EditAnywhere, Category="Lua")
	bool bPersistent;

	int32 GetTop();

	FString LastError;

	int32 InceptionLevel;

	TQueue<FString> InceptionErrors;

	void NewTable();

	void SetMetaTable(int Index);
	void GetMetaTable(int Index);

	void SetField(int Index, const char* FieldName);

	void GetField(int Index, const char* FieldName);

	void NewUObject(UObject* Object);

	void* NewUserData(size_t DataSize);

	void GetGlobal(const char* Name);

	int32 GetFieldFromTree(FString Tree, bool bGlobal = true);

	void SetFieldFromTree(FString Tree, FLuaValue& Value, bool bGlobal = true);

	void SetGlobal(const char* Name);

	void PushValue(int Index);

	void PushGlobalTable();

	bool PCall(int NArgs, FLuaValue& Value, int NRet = 1);
	bool Call(int NArgs, FLuaValue& Value, int NRet = 1);

	void Pop(int32 Amount = 1);

	void PushNil();

	void Unref(int Ref);
	int NewRef();
	void GetRef(int Ref);
	int Next(int Index);

	int GC(int What, int Data = 0);

	int32 ToInteger(int Index);

	void Len(int Index);

	void RawGetI(int Index, int N);
	void RawSetI(int Index, int N);

	void PushCFunction(lua_CFunction Function);

	ULuaState* GetLuaState(UWorld* InWorld);

	bool RunCode(TArray<uint8> Code, FString CodePath, int NRet = 0);
	bool RunCode(FString Code, FString CodePath, int NRet = 0);

	bool RunCodeAsset(ULuaCode* CodeAsset, int NRet = 0);

	FLuaValue CreateLuaTable();

	bool RunFile(FString Filename, bool bIgnoreNonExistent, int NRet = 0);

	static int MetaTableFunctionLuaComponent__index(lua_State *L);
	static int MetaTableFunctionLuaComponent__newindex(lua_State *L);

	static int MetaTableFunctionState__index(lua_State *L);
	static int MetaTableFunctionState__newindex(lua_State *L);

	static int TableFunction_print(lua_State *L);
	static int TableFunction_package_preload(lua_State *L);

	static int MetaTableFunction__call(lua_State *L);

	static int MetaTableFunctionUserData__eq(lua_State *L);

	static int ToByteCode_Writer(lua_State* L, const void* Ptr, size_t Size, void* UserData);

	static TArray<uint8> ToByteCode(FString Code, FString CodePath, FString& ErrorString);

	FLuaValue FromUProperty(void* Buffer, UProperty* Property, bool& bSuccess, int32 Index=0);
	void ToUProperty(void* Buffer, UProperty* Property, FLuaValue Value, bool& bSuccess, int32 Index = 0);

	static ULuaState* GetFromExtraSpace(lua_State *L)
	{
		ULuaState** LuaExtraSpacePtr = (ULuaState**)lua_getextraspace(L);
		return *LuaExtraSpacePtr;
	}

	void Log(FString Message)
	{
		UE_LOG(LogLuaMachine, Log, TEXT("%s"), *Message);
	}

	void LogWarning(FString Message)
	{
		UE_LOG(LogLuaMachine, Warning, TEXT("%s"), *Message);
	}

	void LogError(FString Message)
	{
		UE_LOG(LogLuaMachine, Error, TEXT("%s"), *Message);
	}

	void SetUserDataMetaTable(FLuaValue MetaTable);

protected:
	lua_State* L;
	bool bDisabled;

	UWorld* CurrentWorld;

	FLuaValue UserDataMetaTable;
};
