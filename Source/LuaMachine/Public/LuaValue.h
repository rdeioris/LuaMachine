// Copyright 2018-2020 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ThirdParty/lua/lua.hpp"
#include "Serialization/JsonSerializer.h"
#include "LuaValue.generated.h"

// required for Mac
#ifdef Nil
#undef Nil
#endif

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
	Function,
	Table,
	UFunction,
	UObject,
	Thread,
};

class ULuaState;

USTRUCT(BlueprintType)
struct LUAMACHINE_API FLuaValue
{
	GENERATED_BODY()

	FLuaValue()
	{
		Type = ELuaValueType::Nil;
		Object = nullptr;
		LuaRef = LUA_NOREF;
		LuaState = nullptr;
		Bool = false;
		Integer = 0;
		Number = 0;
	}

	FLuaValue(const FLuaValue& SourceValue);
	FLuaValue& operator = (const FLuaValue &SourceValue);

	FLuaValue(FString InString) : FLuaValue()
	{
		Type = ELuaValueType::String;
		String = InString;
	}

	FLuaValue(const char* InChars, size_t Length) : FLuaValue()
	{
		Type = ELuaValueType::String;
		for (size_t i = 0; i < Length; i++)
		{
			uint16 TChar = (uint16)InChars[i];
			// cleanup garbage
			TChar &= 0xFF;
			// hack for allowing binary data
			if (TChar == 0)
				TChar = 0xffff;
			String += (TCHAR)TChar;
		}
	}

	FLuaValue(TArray<uint8> InBytes) : FLuaValue((const char*)InBytes.GetData(), InBytes.Num())
	{
	}

	FLuaValue(float Value) : FLuaValue()
	{
		Type = ELuaValueType::Number;
		Number = Value;
	}

	FLuaValue(int32 Value) : FLuaValue()
	{
		Type = ELuaValueType::Integer;
		Integer = Value;
	}

	FLuaValue(bool bInBool) : FLuaValue()
	{
		Type = ELuaValueType::Bool;
		Bool = bInBool;
	}

	FLuaValue(UObject* InObject) : FLuaValue()
	{
		if (InObject)
		{
			Type = ELuaValueType::UObject;
			Object = InObject;
		}
	}

	~FLuaValue();

	static FLuaValue Function(FName FunctionName)
	{
		FLuaValue LuaValue;
		LuaValue.Type = ELuaValueType::UFunction;
		LuaValue.FunctionName = FunctionName;
		return LuaValue;
	}

	FString ToString();
	FName ToName();
	int32 ToInteger();
	float ToFloat();
	bool ToBool();

	TArray<uint8> ToBytes();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lua")
	ELuaValueType Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua")
	bool Bool;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua")
	int32 Integer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua")
	float Number;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua")
	FString String;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua")
	UObject* Object;

	UPROPERTY(EditAnywhere, Category="Lua")
	FName FunctionName;

	int LuaRef;

	UPROPERTY()
	ULuaState* LuaState;

	FLuaValue GetField(FString Key);
	FLuaValue SetField(FString Key, FLuaValue Value);

	FLuaValue GetFieldByIndex(int32 Index);
	FLuaValue SetFieldByIndex(int32 Index, FLuaValue Value);

	bool IsReferencedInLuaRegistry() const;

	static FLuaValue FromJsonValue(ULuaState* L, FJsonValue& JsonValue);
	TSharedPtr<FJsonValue> ToJsonValue();

	static FLuaValue FromBase64(FString Base64);
	FString ToBase64();

	void Unref();
};
