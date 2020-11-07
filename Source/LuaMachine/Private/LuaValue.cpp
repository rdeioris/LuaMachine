// Copyright 2018-2020 - Roberto De Ioris

#include "LuaValue.h"
#include "LuaState.h"
#include "Misc/Base64.h"

FString FLuaValue::ToString() const
{
	switch (Type)
	{
	case ELuaValueType::Bool:
		return Bool ? FString(TEXT("true")) : FString(TEXT("false"));
	case ELuaValueType::Integer:
		return FString::FromInt(Integer);
	case ELuaValueType::Number:
		return FString::SanitizeFloat(Number);
	case ELuaValueType::String:
		return String;
	case ELuaValueType::Table:
		return FString::Printf(TEXT("table: %d"), LuaRef);
	case ELuaValueType::Function:
		return FString::Printf(TEXT("function: %d"), LuaRef);
	case ELuaValueType::UObject:
		return Object->GetFullName();
	case ELuaValueType::UFunction:
		return Object ? (FunctionName.ToString() + " @ " + Object->GetClass()->GetPathName()) : FunctionName.ToString();
	case ELuaValueType::Thread:
		return FString::Printf(TEXT("thread: %d"), LuaRef);
	}
	return FString(TEXT("nil"));
}

FName FLuaValue::ToName() const
{
	return FName(*ToString());
}

int32 FLuaValue::ToInteger() const
{
	switch (Type)
	{
	case ELuaValueType::Bool:
		return Bool ? 1 : 0;
	case ELuaValueType::Integer:
		return Integer;
	case ELuaValueType::Number:
		return Number;
	case ELuaValueType::String:
		return FCString::Atoi(*String);
	}
	return 0;
}

float FLuaValue::ToFloat() const
{
	switch (Type)
	{
	case ELuaValueType::Bool:
		return Bool ? 1.0f : 0.0f;
	case ELuaValueType::Integer:
		return Integer;
	case ELuaValueType::Number:
		return Number;
	case ELuaValueType::String:
		return FCString::Atof(*String);
	}
	return 0.0f;
}

bool FLuaValue::ToBool() const
{
	switch (Type)
	{
	case ELuaValueType::Nil:
		return false;
	case ELuaValueType::Bool:
		return Bool;
	case ELuaValueType::Integer:
		return Integer != 0;
	case ELuaValueType::Number:
		return Number != 0;
	}
	return true;
}

void FLuaValue::Unref()
{
	if (Type == ELuaValueType::Table || Type == ELuaValueType::Function || Type == ELuaValueType::Thread)
	{
		if (LuaRef != LUA_NOREF)
		{
			// special case for when the engine is shutting down
#if ENGINE_MINOR_VERSION >= 24
			if (IsEngineExitRequested())
#else
			if (GIsRequestingExit)
#endif
			{
				if (!LuaState->IsValidLowLevel())
				{
					LuaRef = LUA_NOREF;
					return;
				}
			}
			// use UnrefCheck here to support moving of LuaState
			LuaState->UnrefChecked(LuaRef);
		}
		LuaRef = LUA_NOREF;
	}
}

FLuaValue::~FLuaValue()
{
	Unref();
}

FLuaValue::FLuaValue(const FLuaValue& SourceValue)
{
	Type = SourceValue.Type;
	Object = SourceValue.Object;
	LuaRef = SourceValue.LuaRef;
	LuaState = SourceValue.LuaState;
	Bool = SourceValue.Bool;
	Integer = SourceValue.Integer;
	Number = SourceValue.Number;
	String = SourceValue.String;
	FunctionName = SourceValue.FunctionName;

	// make a new reference to the table, to avoid it being destroyed
	if (LuaRef != LUA_NOREF)
	{
		LuaState->GetRef(LuaRef);
		LuaRef = LuaState->NewRef();
	}
}

FLuaValue& FLuaValue::operator = (const FLuaValue& SourceValue)
{
	Type = SourceValue.Type;
	Object = SourceValue.Object;
	LuaRef = SourceValue.LuaRef;
	LuaState = SourceValue.LuaState;
	Bool = SourceValue.Bool;
	Integer = SourceValue.Integer;
	Number = SourceValue.Number;
	String = SourceValue.String;
	FunctionName = SourceValue.FunctionName;

	// make a new reference to the table, to avoid it being destroyed
	if (LuaRef != LUA_NOREF)
	{
		LuaState->GetRef(LuaRef);
		LuaRef = LuaState->NewRef();
	}

	return *this;
}

FLuaValue FLuaValue::SetField(const FString& Key, FLuaValue Value)
{
	if (Type != ELuaValueType::Table)
		return *this;

	if (!LuaState)
		return *this;

	LuaState->FromLuaValue(*this);
	LuaState->FromLuaValue(Value);
	LuaState->SetField(-2, TCHAR_TO_ANSI(*Key));
	LuaState->Pop();
	return *this;
}

FLuaValue FLuaValue::SetField(const FString& Key, lua_CFunction CFunction)
{
	if (Type != ELuaValueType::Table)
		return *this;

	if (!LuaState)
		return *this;

	LuaState->FromLuaValue(*this);
	LuaState->PushCFunction(CFunction);
	LuaState->SetField(-2, TCHAR_TO_ANSI(*Key));
	LuaState->Pop();
	return *this;
}

FLuaValue FLuaValue::SetMetaTable(FLuaValue MetaTable)
{
	if (Type != ELuaValueType::Table || MetaTable.Type != ELuaValueType::Table)
		return *this;

	if (!LuaState)
		return *this;

	LuaState->FromLuaValue(*this);
	LuaState->FromLuaValue(MetaTable);
	LuaState->SetMetaTable(-2);
	LuaState->Pop();

	return *this;
}


FLuaValue FLuaValue::GetField(const FString& Key)
{
	if (Type != ELuaValueType::Table)
		return FLuaValue();

	if (!LuaState)
		return FLuaValue();

	LuaState->FromLuaValue(*this);
	LuaState->GetField(-1, TCHAR_TO_ANSI(*Key));
	FLuaValue ReturnValue = LuaState->ToLuaValue(-1);
	LuaState->Pop(2);
	return ReturnValue;
}

FLuaValue FLuaValue::GetFieldByIndex(int32 Index)
{
	if (Type != ELuaValueType::Table)
		return FLuaValue();

	if (!LuaState)
		return FLuaValue();

	LuaState->FromLuaValue(*this);
	LuaState->RawGetI(-1, Index);
	FLuaValue ReturnValue = LuaState->ToLuaValue(-1);
	LuaState->Pop(2);
	return ReturnValue;
}

FLuaValue FLuaValue::SetFieldByIndex(int32 Index, FLuaValue Value)
{
	if (Type != ELuaValueType::Table)
		return *this;

	if (!LuaState)
		return *this;

	LuaState->FromLuaValue(*this);
	LuaState->FromLuaValue(Value);
	LuaState->RawSetI(-2, Index);
	LuaState->Pop();
	return *this;
}

bool FLuaValue::IsReferencedInLuaRegistry() const
{
	return LuaRef != LUA_NOREF;
}

bool FLuaValue::IsNil() const
{
	return Type == ELuaValueType::Nil;
}

FLuaValue FLuaValue::FromJsonValue(ULuaState* L, FJsonValue& JsonValue)
{
	if (JsonValue.Type == EJson::String)
	{
		return FLuaValue(JsonValue.AsString());
	}
	else if (JsonValue.Type == EJson::Number)
	{
		return FLuaValue((float)JsonValue.AsNumber());
	}
	else if (JsonValue.Type == EJson::Boolean)
	{
		return FLuaValue(JsonValue.AsBool());
	}
	else if (JsonValue.Type == EJson::Array)
	{
		FLuaValue LuaArray = L->CreateLuaTable();
		int32 Index = 1;
		auto JsonValues = JsonValue.AsArray();
		for (auto JsonItem : JsonValues)
		{
			FLuaValue LuaItem;
			if (JsonItem.IsValid())
			{
				LuaItem = FromJsonValue(L, *JsonItem);
			}
			LuaArray.SetFieldByIndex(Index++, LuaItem);
		}
		return LuaArray;
	}
	else if (JsonValue.Type == EJson::Object)
	{
		FLuaValue LuaTable = L->CreateLuaTable();
		auto JsonObject = JsonValue.AsObject();
		for (TPair<FString, TSharedPtr<FJsonValue>> Pair : JsonObject->Values)
		{
			FLuaValue LuaItem;
			if (Pair.Value.IsValid())
			{
				LuaItem = FromJsonValue(L, *Pair.Value);
			}
			LuaTable.SetField(Pair.Key, LuaItem);
		}
		return LuaTable;
	}

	// default to nil
	return FLuaValue();
}

TSharedPtr<FJsonValue> FLuaValue::ToJsonValue()
{
	switch (Type)
	{
	case ELuaValueType::Integer:
		return MakeShared<FJsonValueNumber>(Integer);
	case ELuaValueType::Number:
		return MakeShared<FJsonValueNumber>(Number);
	case ELuaValueType::String:
		return MakeShared<FJsonValueString>(String);
	case ELuaValueType::UFunction:
		return MakeShared<FJsonValueString>(FunctionName.ToString());
	case ELuaValueType::UObject:
		return MakeShared<FJsonValueString>(Object ? Object->GetFullName() : "");
	case ELuaValueType::Table:
	{

		ULuaState* L = LuaState;
		if (!L)
			return MakeShared<FJsonValueNull>();

		bool bIsArray = true;

		TArray<TPair<FLuaValue, FLuaValue>> Items;
		L->FromLuaValue(*this); // push the table
		L->PushNil(); // first key
		while (L->Next(-2))
		{
			auto Key = L->ToLuaValue(-2);
			auto Value = L->ToLuaValue(-1);
			Items.Add(TPair<FLuaValue, FLuaValue>(Key, Value));
			if (Key.Type != ELuaValueType::Integer)
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
				FLuaValue Item = GetFieldByIndex(Index++);
				if (Item.Type == ELuaValueType::Nil)
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

TArray<uint8> FLuaValue::ToBytes() const
{
	TArray<uint8> Bytes;
	if (Type != ELuaValueType::String)
		return Bytes;

	int32 StringLength = String.Len();
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

FLuaValue FLuaValue::FromBase64(const FString& Base64)
{
	TArray<uint8> Bytes;
	FBase64::Decode(Base64, Bytes);
	return FLuaValue(Bytes);
}

FString FLuaValue::ToBase64() const
{
	return FBase64::Encode(ToBytes());
}