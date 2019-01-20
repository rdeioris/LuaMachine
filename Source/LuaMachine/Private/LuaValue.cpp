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
	case ELuaValueType::Thread:
		return FString::Printf(TEXT("thread: %d"), LuaRef);
	}
	return FString(TEXT("nil"));
}

FName FLuaValue::ToName()
{
	return FName(*ToString());
}

int32 FLuaValue::ToInteger()
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

float FLuaValue::ToFloat()
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

bool FLuaValue::ToBool()
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

FLuaValue::~FLuaValue()
{
	if (Type == ELuaValueType::Table || Type == ELuaValueType::Function || Type == ELuaValueType::Thread)
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
	if ((Type == ELuaValueType::Table || Type == ELuaValueType::Function || Type == ELuaValueType::Thread) && LuaRef != LUA_NOREF)
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
	if ((Type == ELuaValueType::Table || Type == ELuaValueType::Function || Type == ELuaValueType::Thread) && LuaRef != LUA_NOREF)
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
		return FLuaValue();

	if (!LuaState)
		return FLuaValue();

	LuaState->FromLuaValue(*this);
	LuaState->GetField(-1, TCHAR_TO_UTF8(*Key));
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
