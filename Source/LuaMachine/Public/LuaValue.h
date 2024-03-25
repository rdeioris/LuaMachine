// Copyright 2018-2023 - Roberto De Ioris

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
	MulticastDelegate,
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
		MulticastScriptDelegate = nullptr;
	}

	FLuaValue(const FLuaValue& SourceValue);
	FLuaValue& operator = (const FLuaValue &SourceValue);

	FLuaValue(const FString& InString) : FLuaValue()
	{
		Type = ELuaValueType::String;
		String = InString;
	}

	FLuaValue(const char* InChars) : FLuaValue(FString(InChars))
	{
	}

	FLuaValue(const TCHAR* InChars) : FLuaValue(FString(InChars))
	{
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
			{
				TChar = 0xffff;
			}
			String += (TCHAR)TChar;
		}
	}

	FLuaValue(const TArray<uint8>& InBytes) : FLuaValue(reinterpret_cast<const char*>(InBytes.GetData()), InBytes.Num())
	{
	}

	FLuaValue(const double Value) : FLuaValue()
	{
		Type = ELuaValueType::Number;
		Number = Value;
	}

	FLuaValue(const float Value) : FLuaValue()
	{
		Type = ELuaValueType::Number;
		Number = Value;
	}

	FLuaValue(const int64 Value) : FLuaValue()
	{
		Type = ELuaValueType::Integer;
		Integer = Value;
	}

	FLuaValue(const int32 Value) : FLuaValue()
	{
		Type = ELuaValueType::Integer;
		Integer = Value;
	}

	FLuaValue(const bool bInBool) : FLuaValue()
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

	static FLuaValue FunctionOfObject(UObject* InObject, FName FunctionName)
	{
		FLuaValue LuaValue;
		LuaValue.Type = ELuaValueType::UFunction;
		LuaValue.Object = InObject;
		LuaValue.FunctionName = FunctionName;
		return LuaValue;
	}

	FString ToString() const;
	FName ToName() const;
	int64 ToInteger() const;
	double ToFloat() const;
	bool ToBool() const;

	TArray<uint8> ToBytes() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua")
	ELuaValueType Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua")
	bool Bool;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua")
	int64 Integer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua")
	double Number;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua")
	FString String;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua")
	UObject* Object;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua")
	FName FunctionName;

	int LuaRef;

	TWeakObjectPtr<ULuaState> LuaState;

	FLuaValue GetField(const FString& Key);
	FLuaValue SetField(const FString& Key, FLuaValue Value);

	FLuaValue SetField(const FString& Key, lua_CFunction CFunction);

	FLuaValue GetFieldByIndex(const int32 Index);
	FLuaValue SetFieldByIndex(const int32 Index, FLuaValue Value);

	FLuaValue SetMetaTable(FLuaValue MetaTable);

	bool IsReferencedInLuaRegistry() const;

	static FLuaValue FromJsonValue(ULuaState* L, FJsonValue& JsonValue);
	TSharedPtr<FJsonValue> ToJsonValue();

	static FLuaValue FromBase64(const FString& Base64);
	FString ToBase64() const;

	bool IsNil() const;

	void Unref();

	FMulticastScriptDelegate* MulticastScriptDelegate = nullptr;
};
