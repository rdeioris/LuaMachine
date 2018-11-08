// Copyright 2018 - Roberto De Ioris

#include "LuaValue.h"
#include "LuaState.h"

FString FLuaValue::ToString()
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
		return FunctionName.ToString();
	}
	return FString(TEXT("None"));
}

FLuaValue::~FLuaValue()
{
	if (Type == ELuaValueType::Table || Type == ELuaValueType::Function)
	{
		if (LuaRef != LUA_NOREF)
		{
			LuaState->Unref(LuaRef);
		}
	}
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
	if ((Type == ELuaValueType::Table || Type == ELuaValueType::Function) && LuaRef != LUA_NOREF)
	{
		LuaState->GetRef(LuaRef);
		LuaRef = LuaState->NewRef();
	}
}

FLuaValue& FLuaValue::operator = (const FLuaValue &SourceValue)
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
	if ((Type == ELuaValueType::Table || Type == ELuaValueType::Function) && LuaRef != LUA_NOREF)
	{
		LuaState->GetRef(LuaRef);
		LuaRef = LuaState->NewRef();
	}

	return *this;
}

FLuaValue FLuaValue::SetField(FString Key, FLuaValue Value)
{
	if (Type != ELuaValueType::Table)
		return *this;

	if (!LuaState)
		return *this;

	LuaState->FromLuaValue(*this);
	LuaState->FromLuaValue(Value);
	LuaState->SetField(-2, TCHAR_TO_UTF8(*Key));
	LuaState->Pop();
	return *this;
}

FLuaValue FLuaValue::GetField(FString Key)
{
	if (Type != ELuaValueType::Table)
		return *this;

	if (!LuaState)
		return *this;

	LuaState->FromLuaValue(*this);
	LuaState->GetField(-1, TCHAR_TO_UTF8(*Key));
	FLuaValue ReturnValue = LuaState->ToLuaValue(-1);
	LuaState->Pop(2);
	return ReturnValue;
}

FLuaValue FLuaValue::GetFieldByIndex(int32 Index)
{
	if (Type != ELuaValueType::Table)
		return *this;

	if (!LuaState)
		return *this;

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