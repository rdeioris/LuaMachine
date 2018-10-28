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
	UObject* Context;
	UFunction* Function;

	FLuaUserData(UObject* InObject)
	{
		Type = ELuaValueType::Object;
		Context = InObject;
	}

	FLuaUserData(UObject* InObject, UFunction* InFunction)
	{
		Type = ELuaValueType::Function;
		Context = InObject;
		Function = InFunction;
	}
};


UCLASS(Abstract, EditInlineNew, BlueprintType, Blueprintable)
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
	FString OverridePackagePath;

	UPROPERTY(EditAnywhere)
	FString OverridePackageCPath;

	UFUNCTION(BlueprintImplementableEvent, Category = "Lua", meta = (DisplayName = "Lua Error"))
	void ReceiveLuaError(const FString& Message);

	void FromLuaValue(FLuaValue& LuaValue);
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

	int32 GetFieldFromTree(FString Tree, bool bGlobal=true);

	void SetFieldFromTree(FString Tree, FLuaValue& Value);

	void SetGlobal(const char* Name);

	void PushValue(int Index);

	void PushGlobalTable();

	bool PCall(int NArgs, FLuaValue& Value, int NRet=1);
	bool Call(int NArgs, FLuaValue& Value, int NRet=1);

	void Pop(int32 Amount = 1);

	void PushNil();

	void Unref(int Ref);
	int NewRef();
	void GetRef(int Ref);
	int Next(int Index);

	void PushCFunction(lua_CFunction Function);
	
	ULuaState* GetLuaState(UWorld* InWorld);

	bool RunCode(FString Code, FString CodePath, int NRet=0);

	static int MetaTableFunctionLuaComponent__index(lua_State *L);
	static int MetaTableFunctionLuaComponent__newindex(lua_State *L);

	static int MetaTableFunctionState__index(lua_State *L);
	static int MetaTableFunctionState__newindex(lua_State *L);

	static int TableFunction_print(lua_State *L);
	static int TableFunction_package_preload(lua_State *L);

	static int MetaTableFunction__call(lua_State *L);
	static int LuaPCallErrorHandler(lua_State *L);

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
