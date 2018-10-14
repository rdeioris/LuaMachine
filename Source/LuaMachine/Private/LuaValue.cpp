// Fill out your copyright notice in the Description page of Project Settings.

#include "LuaValue.h"

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
