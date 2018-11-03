// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "lua/lua.hpp"
#include "LuaValue.h"
#include "LuaCode.h"
#include "Runtime/Core/Public/Containers/Queue.h"
#include "LuaState.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogLuaMachine, Log, All);

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

	UPROPERTY(EditAnywhere)
		ULuaCode* LuaCodeAsset;

	UPROPERTY(EditAnywhere)
		FString LuaFilename;

	UPROPERTY(EditAnywhere)
		TMap<FString, FLuaValue> Table;

	UPROPERTY(EditAnywhere)
		TMap<FString, ULuaCode*> RequireTable;

	UPROPERTY(EditAnywhere)
		bool bLuaOpenLibs;

	UPROPERTY(EditAnywhere)
		bool bAddProjectContentDirToPackagePath;

	UPROPERTY(EditAnywhere)
		TArray<FString> AppendProjectContentDirSubDir;

	UPROPERTY(EditAnywhere)
		FString OverridePackagePath;

	UPROPERTY(EditAnywhere)
		FString OverridePackageCPath;

	UFUNCTION(BlueprintImplementableEvent, Category = "Lua", meta = (DisplayName = "Lua Error"))
		void ReceiveLuaError(const FString& Message);

	void FromLuaValue(FLuaValue& LuaValue, UObject* CallContext = nullptr);
	FLuaValue ToLuaValue(int Index);

	UPROPERTY(EditAnywhere)
		bool bLogError;

	int32 GetTop();

	FString LastError;

	int32 InceptionLevel;

	TQueue<FString> InceptionErrors;

	void NewTable();

	void SetMetaTable(int Index);

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

	void RawGetI(int Index, int N);

	void PushCFunction(lua_CFunction Function);

	ULuaState* GetLuaState(UWorld* InWorld);

	bool RunCode(TArray<uint8> Code, FString CodePath, int NRet = 0);
	bool RunCode(FString Code, FString CodePath, int NRet = 0);

	bool RunCodeAsset(ULuaCode* CodeAsset, int NRet = 0);

	bool RunFile(FString Filename, bool bIgnoreNonExistent, bool& bHasError, int NRet = 0);

	static int MetaTableFunctionLuaComponent__index(lua_State *L);
	static int MetaTableFunctionLuaComponent__newindex(lua_State *L);

	static int MetaTableFunctionState__index(lua_State *L);
	static int MetaTableFunctionState__newindex(lua_State *L);

	static int TableFunction_print(lua_State *L);
	static int TableFunction_package_preload(lua_State *L);

	static int MetaTableFunction__call(lua_State *L);

	static int ToByteCode_Writer(lua_State* L, const void* Ptr, size_t Size, void* UserData);

	static TArray<uint8> ToByteCode(FString Code, FString CodePath, FString& ErrorString);

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

protected:
	lua_State* L;
	bool bDisabled;

	UWorld* CurrentWorld;
};
