// Fill out your copyright notice in the Description page of Project Settings.

#include "LuaBlueprintFunctionLibrary.h"



FLuaValue ULuaBlueprintFunctionLibrary::LuaCreateString(FString String)
{
	FLuaValue LuaString;
	LuaString.Type = ELuaValueType::String;
	LuaString.String = String;
	return LuaString;
}

FString ULuaBlueprintFunctionLibrary::Conv_LuaValueToString(FLuaValue Value)
{
	switch (Value.Type)
	{
	case ELuaValueType::Bool:
		return Value.Bool ? FString(TEXT("true")) : FString(TEXT("false"));
	case ELuaValueType::Integer:
		return FString::FromInt(Value.Integer);
	case ELuaValueType::Number:
		return FString::SanitizeFloat(Value.Number);
	case ELuaValueType::String:
		return Value.String;
	}
	return FString(TEXT("None"));
}