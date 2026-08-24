// Copyright 2018-2024 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "LuaVMIncludes.h"
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
	Lambda,
};

class ULuaState;
struct FLuaValueOrError;

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
		Lambda = nullptr;
	}

	FLuaValue(const FLuaValue& SourceValue);
	FLuaValue& operator = (const FLuaValue& SourceValue);

	// Move operations: steal the source's LuaRef rather than creating a new registry entry.
	// Both must Unref() *this first — same reason as copy assignment — to avoid leaking an
	// existing registry ref when overwriting a non-nil FLuaValue.
	FLuaValue(FLuaValue&& SourceValue);
	FLuaValue& operator = (FLuaValue&& SourceValue);

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

	FLuaValue(TFunction<FLuaValueOrError(TArray<FLuaValue>)> InLambda);

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

	static FLuaValue NewLambda(TFunction<FLuaValueOrError(TArray<FLuaValue>)> InLambda);

	FString ToString() const;
	FName ToName() const;
	int64 ToInteger() const;
	double ToFloat() const;
	bool ToBool() const;

	TArray<uint8> ToBytes() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lua")
	ELuaValueType Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lua")
	bool Bool;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lua")
	int64 Integer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lua")
	double Number;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lua")
	FString String;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lua")
	UObject* Object;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lua")
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
	TSharedPtr<TFunction<FLuaValueOrError(TArray<FLuaValue>)>> Lambda = nullptr;
};

struct FLuaValueOrError
{
public:
	FLuaValueOrError()
	{
		// default to nil
		LuaValueOrError.Set<FLuaValue>(FLuaValue());
	}

	FLuaValueOrError(FLuaValue LuaValue)
	{
		LuaValueOrError.Set<FLuaValue>(LuaValue);
	}

	FLuaValueOrError(const FString& Error)
	{
		LuaValueOrError.Set<FString>(Error);
	}

	bool IsError() const
	{
		return LuaValueOrError.IsType<FString>();
	}

	FString GetError() const
	{
		return LuaValueOrError.Get<FString>();
	}

	FLuaValue GetLuaValue() const
	{
		return LuaValueOrError.Get<FLuaValue>();
	}

protected:
	TVariant<struct FLuaValue, FString> LuaValueOrError;
};