// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "lua/lua.hpp"
#include "LuaValue.h"
#include "LuaCode.h"
#include "LuaState.generated.h"

/**
 *
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
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
	TMap<FString, FLuaValue> Table;

	UPROPERTY(EditAnywhere)
	bool bLuaOpenLibs;

	void FromLuaValue(FLuaValue& LuaValue);
	FLuaValue ToLuaValue(int Index);

	void NewTable();

	void SetMetaTable(int Index);

	void SetField(int Index, const char* FieldName);

	void GetField(int Index, const char* FieldName);

	void NewActor(AActor* Actor);

	void GetGlobal(const char* Name);

	int32 GetFunctionFromTree(FString Tree);

	void SetGlobal(const char* Name);

	void PushValue(int Index);

	FLuaValue PCall(int NArgs);

	void Pop(int32 Amount = 1);

	void PushNil();

	void PushCFunction(lua_CFunction Function);
	
	ULuaState* GetLuaState(UWorld* InWorld);

	static int MetaTableFunctionActor__index(lua_State *L);
	static int MetaTableFunctionActor__call(lua_State *L);

	static int MetaTableFunctionState__call(lua_State *L);

protected:
	lua_State* L;
	bool bDisabled;

	static void Internal_FromLuaValue(lua_State *L, FLuaValue& LuaValue);
	static FLuaValue Internal_ToLuaValue(lua_State *L, int Index);

	UWorld* CurrentWorld;
};
