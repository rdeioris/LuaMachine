// Copyright 2018-2023 - Roberto De Ioris
// Portions Copyright 2025 - Fusion Point Studios, Inc.

#include "LuauValue.h"
#include "LuauState.h"
#include "Misc/Base64.h"

FString FLuauValue::ToString() const
{
	switch (Type)
	{
	case ELuauValueType::Bool:
		return Bool ? FString(TEXT("true")) : FString(TEXT("false"));
	case ELuauValueType::Integer:
		return FString::Printf(TEXT("%lld"), Integer);
	case ELuauValueType::Number:
		return FString::SanitizeFloat(Number);
	case ELuauValueType::String:
		return String;
	case ELuauValueType::Table:
		return FString::Printf(TEXT("table: %d"), LuauRef);
	case ELuauValueType::Function:
		return FString::Printf(TEXT("function: %d"), LuauRef);
	case ELuauValueType::UObject:
		return Object->GetFullName();
	case ELuauValueType::UFunction:
		return Object ? (FunctionName.ToString() + " @ " + Object->GetClass()->GetPathName()) : FunctionName.ToString();
	case ELuauValueType::Thread:
		return FString::Printf(TEXT("thread: %d"), LuauRef);
	}
	return FString(TEXT("nil"));
}

FName FLuauValue::ToName() const
{
	return FName(*ToString());
}

int64 FLuauValue::ToInteger() const
{
	switch (Type)
	{
	case ELuauValueType::Bool:
		return Bool ? 1 : 0;
	case ELuauValueType::Integer:
		return Integer;
	case ELuauValueType::Number:
		return Number;
	case ELuauValueType::String:
		return FCString::Atoi(*String);
	}
	return 0;
}

double FLuauValue::ToFloat() const
{
	switch (Type)
	{
	case ELuauValueType::Bool:
		return Bool ? 1.0 : 0.0;
	case ELuauValueType::Integer:
		return Integer;
	case ELuauValueType::Number:
		return Number;
	case ELuauValueType::String:
		return FCString::Atod(*String);
	}
	return 0.0;
}

bool FLuauValue::ToBool() const
{
	switch (Type)
	{
	case ELuauValueType::Nil:
		return false;
	case ELuauValueType::Bool:
		return Bool;
	case ELuauValueType::Integer:
		return Integer != 0;
	case ELuauValueType::Number:
		return Number != 0;
	}
	return true;
}

void FLuauValue::Unref()
{
	if (!LuauState.IsValid())
	{
		LuauRef = LUA_NOREF;
		return;
	}

	if (Type == ELuauValueType::Table || Type == ELuauValueType::Function || Type == ELuauValueType::Thread)
	{
		if (LuauRef != LUA_NOREF)
		{
			// special case for when the engine is shutting down
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 24
			if (IsEngineExitRequested())
#else
			if (GIsRequestingExit)
#endif
			{
				if (!LuauState->IsValidLowLevel())
				{
					LuauRef = LUA_NOREF;
					return;
				}
			}
			// use UnrefCheck here to support moving of LuauState
			LuauState->UnrefChecked(LuauRef);
		}
		LuauRef = LUA_NOREF;
	}
}

FLuauValue::~FLuauValue()
{
	Unref();
}

FLuauValue::FLuauValue(const FLuauValue& SourceValue)
{
	Type = SourceValue.Type;
	Object = SourceValue.Object;
	LuauRef = SourceValue.LuauRef;
	LuauState = SourceValue.LuauState;
	Bool = SourceValue.Bool;
	Integer = SourceValue.Integer;
	Number = SourceValue.Number;
	String = SourceValue.String;
	FunctionName = SourceValue.FunctionName;
	MulticastScriptDelegate = SourceValue.MulticastScriptDelegate;

	// make a new reference to the table, to avoid it being destroyed
	if (LuauRef != LUA_NOREF)
	{
		LuauState->GetRef(LuauRef);
		LuauRef = LuauState->NewRef();
	}
}

FLuauValue& FLuauValue::operator = (const FLuauValue& SourceValue)
{
	Type = SourceValue.Type;
	Object = SourceValue.Object;
	LuauRef = SourceValue.LuauRef;
	LuauState = SourceValue.LuauState;
	Bool = SourceValue.Bool;
	Integer = SourceValue.Integer;
	Number = SourceValue.Number;
	String = SourceValue.String;
	FunctionName = SourceValue.FunctionName;
	MulticastScriptDelegate = SourceValue.MulticastScriptDelegate;

	// make a new reference to the table, to avoid it being destroyed
	if (LuauRef != LUA_NOREF)
	{
		LuauState->GetRef(LuauRef);
		LuauRef = LuauState->NewRef();
	}

	return *this;
}

FLuauValue FLuauValue::SetField(const FString& Key, FLuauValue Value)
{
	if (Type != ELuauValueType::Table)
		return *this;

	if (!LuauState.IsValid())
		return *this;

	LuauState->FromLuauValue(*this);
	LuauState->FromLuauValue(Value);
	LuauState->SetField(-2, TCHAR_TO_ANSI(*Key));
	LuauState->Pop();
	return *this;
}

FLuauValue FLuauValue::SetField(const FString& Key, lua_CFunction CFunction)
{
	if (Type != ELuauValueType::Table)
		return *this;

	if (!LuauState.IsValid())
		return *this;

	LuauState->FromLuauValue(*this);
	LuauState->PushCFunction(CFunction);
	LuauState->SetField(-2, TCHAR_TO_ANSI(*Key));
	LuauState->Pop();
	return *this;
}

FLuauValue FLuauValue::SetMetaTable(FLuauValue MetaTable)
{
	if (Type != ELuauValueType::Table || MetaTable.Type != ELuauValueType::Table)
		return *this;

	if (!LuauState.IsValid())
		return *this;

	LuauState->FromLuauValue(*this);
	LuauState->FromLuauValue(MetaTable);
	LuauState->SetMetaTable(-2);
	LuauState->Pop();

	return *this;
}


FLuauValue FLuauValue::GetField(const FString& Key)
{
	if (Type != ELuauValueType::Table)
		return FLuauValue();

	if (!LuauState.IsValid())
		return FLuauValue();

	LuauState->FromLuauValue(*this);
	LuauState->GetField(-1, TCHAR_TO_ANSI(*Key));
	FLuauValue ReturnValue = LuauState->ToLuauValue(-1);
	LuauState->Pop(2);
	return ReturnValue;
}

FLuauValue FLuauValue::GetFieldByIndex(const int32 Index)
{
	if (Type != ELuauValueType::Table)
	{
		return FLuauValue();
	}

	if (!LuauState.IsValid())
	{
		return FLuauValue();
	}

	LuauState->FromLuauValue(*this);
	LuauState->RawGetI(-1, Index);
	FLuauValue ReturnValue = LuauState->ToLuauValue(-1);
	LuauState->Pop(2);
	return ReturnValue;
}

FLuauValue FLuauValue::SetFieldByIndex(const int32 Index, FLuauValue Value)
{
	if (Type != ELuauValueType::Table)
	{
		return *this;
	}

	if (!LuauState.IsValid())
	{
		return *this;
	}

	LuauState->FromLuauValue(*this);
	LuauState->FromLuauValue(Value);
	LuauState->RawSetI(-2, Index);
	LuauState->Pop();
	return *this;
}

bool FLuauValue::IsReferencedInLuauRegistry() const
{
	return LuauRef != LUA_NOREF;
}

bool FLuauValue::IsNil() const
{
	return Type == ELuauValueType::Nil;
}

FLuauValue FLuauValue::FromJsonValue(ULuauState* L, FJsonValue& JsonValue)
{
	if (JsonValue.Type == EJson::String)
	{
		return FLuauValue(JsonValue.AsString());
	}
	else if (JsonValue.Type == EJson::Number)
	{
		return FLuauValue(JsonValue.AsNumber());
	}
	else if (JsonValue.Type == EJson::Boolean)
	{
		return FLuauValue(JsonValue.AsBool());
	}
	else if (JsonValue.Type == EJson::Array)
	{
		FLuauValue LuauArray = L->CreateLuauTable();
		int32 Index = 1;
		auto JsonValues = JsonValue.AsArray();
		for (auto JsonItem : JsonValues)
		{
			FLuauValue LuauItem;
			if (JsonItem.IsValid())
			{
				LuauItem = FromJsonValue(L, *JsonItem);
			}
			LuauArray.SetFieldByIndex(Index++, LuauItem);
		}
		return LuauArray;
	}
	else if (JsonValue.Type == EJson::Object)
	{
		FLuauValue LuauTable = L->CreateLuauTable();
		auto JsonObject = JsonValue.AsObject();
		for (TPair<FString, TSharedPtr<FJsonValue>> Pair : JsonObject->Values)
		{
			FLuauValue LuauItem;
			if (Pair.Value.IsValid())
			{
				LuauItem = FromJsonValue(L, *Pair.Value);
			}
			LuauTable.SetField(Pair.Key, LuauItem);
		}
		return LuauTable;
	}

	// default to nil
	return FLuauValue();
}

TSharedPtr<FJsonValue> FLuauValue::ToJsonValue()
{
	switch (Type)
	{
	case ELuauValueType::Integer:
		return MakeShared<FJsonValueNumber>(Integer);
	case ELuauValueType::Number:
		return MakeShared<FJsonValueNumber>(Number);
	case ELuauValueType::String:
		return MakeShared<FJsonValueString>(String);
	case ELuauValueType::UFunction:
		return MakeShared<FJsonValueString>(FunctionName.ToString());
	case ELuauValueType::UObject:
		return MakeShared<FJsonValueString>(Object ? Object->GetFullName() : "");
	case ELuauValueType::Table:
	{

		ULuauState* L = LuauState.Get();
		if (!L)
			return MakeShared<FJsonValueNull>();

		bool bIsArray = true;

		TArray<TPair<FLuauValue, FLuauValue>> Items;
		L->FromLuauValue(*this); // push the table
		L->PushNil(); // first key
		while (L->Next(-2))
		{
			auto Key = L->ToLuauValue(-2);
			auto Value = L->ToLuauValue(-1);
			Items.Add(TPair<FLuauValue, FLuauValue>(Key, Value));
			if (Key.Type != ELuauValueType::Integer)
			{
				bIsArray = false;
			}
			L->Pop(); // pop the value
		}
		L->Pop(); // pop the table

		// check if it is a valid lua "array"
		if (bIsArray)
		{
			TArray<TSharedPtr<FJsonValue>> JsonValues;
			int32 Index = 1;
			for (;;)
			{
				FLuauValue Item = GetFieldByIndex(Index++);
				if (Item.Type == ELuauValueType::Nil)
					break;
				JsonValues.Add(Item.ToJsonValue());
			}
			return MakeShared<FJsonValueArray>(JsonValues);
		}

		auto JsonObject = MakeShared<FJsonObject>();
		for (auto Pair : Items)
		{
			JsonObject->SetField(Pair.Key.ToString(), Pair.Value.ToJsonValue());
		}
		auto JsonValueObject = MakeShared<FJsonValueObject>(JsonObject);
		return JsonValueObject;
	}
	return MakeShared<FJsonValueNull>();
	}

	return MakeShared<FJsonValueNull>();
}

TArray<uint8> FLuauValue::ToBytes() const
{
	TArray<uint8> Bytes;
	if (Type != ELuauValueType::String)
		return Bytes;

	const int32 StringLength = String.Len();
	Bytes.AddUninitialized(StringLength);
	for (int32 i = 0; i < StringLength; i++)
	{
		uint16 CharValue = (uint16)String[i];
		if (CharValue == 0xffff)
		{
			Bytes[i] = 0;
		}
		else
		{
			Bytes[i] = (uint8)String[i];
		}
	}

	return Bytes;
}

FLuauValue FLuauValue::FromBase64(const FString& Base64)
{
	TArray<uint8> Bytes;
	FBase64::Decode(Base64, Bytes);
	return FLuauValue(Bytes);
}

FString FLuauValue::ToBase64() const
{
	return FBase64::Encode(ToBytes());
}