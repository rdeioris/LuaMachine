// Fill out your copyright notice in the Description page of Project Settings.

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