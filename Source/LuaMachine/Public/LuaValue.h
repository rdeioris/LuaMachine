// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "lua/lua.hpp"
#include "LuaValue.generated.h"


/**
 *
 */

UENUM(BlueprintType)
enum class ELuaValueType : uint8
{
	Nil,
	Bool,
	Integer,
	Number,
	String,
	UserData,
	Function,
	Thread,
	Table,
	Object,
};

USTRUCT(BlueprintType)
struct LUAMACHINE_API FLuaValue
{
	GENERATED_BODY()

	FLuaValue()
	{
		Type = ELuaValueType::Nil;
		Object = nullptr;
		TableRef = LUA_NOREF;
	}

	FLuaValue(FString InString) : Object(nullptr), TableRef(LUA_NOREF)
	{
		Type = ELuaValueType::String;
		String = InString;
	}

	FLuaValue(int32 Value) : Object(nullptr), TableRef(LUA_NOREF)
	{
		Type = ELuaValueType::Integer;
		Integer = Value;
	}

	FLuaValue(UObject* InObject) : TableRef(LUA_NOREF)
	{
		Type = ELuaValueType::Object;
		Object = InObject;
	}

	static FLuaValue Function(FName FunctionName)
	{
		FLuaValue LuaValue;
		LuaValue.Type = ELuaValueType::Function;
		LuaValue.FunctionName = FunctionName;
		LuaValue.Object = nullptr;
		LuaValue.TableRef = LUA_NOREF;
		return LuaValue;
	}

	FString ToString();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ELuaValueType Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool Bool;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Integer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Number;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString String;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UObject* Object;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName FunctionName;

	int TableRef;
};
