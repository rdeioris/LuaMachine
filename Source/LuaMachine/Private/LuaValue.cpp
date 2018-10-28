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
		return FString("{}");
	case ELuaValueType::Object:
		return Object->GetFullName();
	}
	return FString(TEXT("None"));
}

FLuaValue::~FLuaValue()
{
	if (Type == ELuaValueType::Table)
	{
		if (TableRef != LUA_NOREF)
		{
			LuaState->Unref(TableRef);
		}
	}
}

FLuaValue::FLuaValue(const FLuaValue& SourceValue)
{
	Type = SourceValue.Type;
	Object = SourceValue.Object;
	TableRef = SourceValue.TableRef;
	LuaState = SourceValue.LuaState;
	Bool = SourceValue.Bool;
	Integer = SourceValue.Integer;
	Number = SourceValue.Number;
	String = SourceValue.String;
	FunctionName = SourceValue.FunctionName;

	// make a new reference to the table, to avoid it being destroyed
	if (Type == ELuaValueType::Table && TableRef != LUA_NOREF)
	{
		LuaState->GetRef(TableRef);
		TableRef = LuaState->NewRef();
	}
}

FLuaValue& FLuaValue::operator = (const FLuaValue &SourceValue)
{
	Type = SourceValue.Type;
	Object = SourceValue.Object;
	TableRef = SourceValue.TableRef;
	LuaState = SourceValue.LuaState;
	Bool = SourceValue.Bool;
	Integer = SourceValue.Integer;
	Number = SourceValue.Number;
	String = SourceValue.String;
	FunctionName = SourceValue.FunctionName;

	// make a new reference to the table, to avoid it being destroyed
	if (Type == ELuaValueType::Table && TableRef != LUA_NOREF)
	{
		LuaState->GetRef(TableRef);
		TableRef = LuaState->NewRef();
	}

	return *this;
}