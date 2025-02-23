// Copyright 2018-2023 - Roberto De Ioris
// Portions Copyright 2025 - Fusion Point Studios, Inc.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ThirdParty/LuauLibrary/VM/include/lua.h"
#include "Serialization/JsonSerializer.h"
#include "LuauValue.generated.h"

// required for Mac
#ifdef Nil
#undef Nil
#endif

/**
 *
 */

UENUM(BlueprintType)
enum class ELuauValueType : uint8
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

class ULuauState;

USTRUCT(BlueprintType)
struct LUAUMACHINE_API FLuauValue
{
	GENERATED_BODY()

	FLuauValue()
	{
		Type = ELuauValueType::Nil;
		Object = nullptr;
		LuauRef = LUA_NOREF;
		LuauState = nullptr;
		Bool = false;
		Integer = 0;
		Number = 0;
		MulticastScriptDelegate = nullptr;
	}

	FLuauValue(const FLuauValue& SourceValue);
	FLuauValue& operator = (const FLuauValue &SourceValue);

	FLuauValue(const FString& InString) : FLuauValue()
	{
		Type = ELuauValueType::String;
		String = InString;
	}

	FLuauValue(const char* InChars) : FLuauValue(FString(InChars))
	{
	}

	FLuauValue(const TCHAR* InChars) : FLuauValue(FString(InChars))
	{
	}

	FLuauValue(const char* InChars, size_t Length) : FLuauValue()
	{
		Type = ELuauValueType::String;
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

	FLuauValue(const TArray<uint8>& InBytes) : FLuauValue(reinterpret_cast<const char*>(InBytes.GetData()), InBytes.Num())
	{
	}

	FLuauValue(const double Value) : FLuauValue()
	{
		Type = ELuauValueType::Number;
		Number = Value;
	}

	FLuauValue(const float Value) : FLuauValue()
	{
		Type = ELuauValueType::Number;
		Number = Value;
	}

	FLuauValue(const int64 Value) : FLuauValue()
	{
		Type = ELuauValueType::Integer;
		Integer = Value;
	}

	FLuauValue(const int32 Value) : FLuauValue()
	{
		Type = ELuauValueType::Integer;
		Integer = Value;
	}

	FLuauValue(const bool bInBool) : FLuauValue()
	{
		Type = ELuauValueType::Bool;
		Bool = bInBool;
	}

	FLuauValue(UObject* InObject) : FLuauValue()
	{
		if (InObject)
		{
			Type = ELuauValueType::UObject;
			Object = InObject;
		}
	}

	~FLuauValue();

	static FLuauValue Function(FName FunctionName)
	{
		FLuauValue LuauValue;
		LuauValue.Type = ELuauValueType::UFunction;
		LuauValue.FunctionName = FunctionName;
		return LuauValue;
	}

	static FLuauValue FunctionOfObject(UObject* InObject, FName FunctionName)
	{
		FLuauValue LuauValue;
		LuauValue.Type = ELuauValueType::UFunction;
		LuauValue.Object = InObject;
		LuauValue.FunctionName = FunctionName;
		return LuauValue;
	}

	FString ToString() const;
	FName ToName() const;
	int64 ToInteger() const;
	double ToFloat() const;
	bool ToBool() const;

	TArray<uint8> ToBytes() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Luau")
	ELuauValueType Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Luau")
	bool Bool;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Luau")
	int64 Integer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Luau")
	double Number;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Luau")
	FString String;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Luau")
	UObject* Object;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Luau")
	FName FunctionName;

	int LuauRef;

	TWeakObjectPtr<ULuauState> LuauState;

	FLuauValue GetField(const FString& Key);
	FLuauValue SetField(const FString& Key, FLuauValue Value);

	FLuauValue SetField(const FString& Key, lua_CFunction CFunction);

	FLuauValue GetFieldByIndex(const int32 Index);
	FLuauValue SetFieldByIndex(const int32 Index, FLuauValue Value);

	FLuauValue SetMetaTable(FLuauValue MetaTable);

	bool IsReferencedInLuauRegistry() const;

	static FLuauValue FromJsonValue(ULuauState* L, FJsonValue& JsonValue);
	TSharedPtr<FJsonValue> ToJsonValue();

	static FLuauValue FromBase64(const FString& Base64);
	FString ToBase64() const;

	bool IsNil() const;

	void Unref();

	FMulticastScriptDelegate* MulticastScriptDelegate = nullptr;
};
