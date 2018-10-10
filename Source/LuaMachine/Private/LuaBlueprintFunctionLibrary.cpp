// Fill out your copyright notice in the Description page of Project Settings.

#include "LuaBlueprintFunctionLibrary.h"
#include "LuaMachine.h"



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

int32 ULuaBlueprintFunctionLibrary::Conv_LuaValueToInt(FLuaValue Value)
{
	switch (Value.Type)
	{
	case ELuaValueType::Bool:
		return Value.Bool ? 1 : 0;
	case ELuaValueType::Integer:
		return Value.Integer;
	case ELuaValueType::Number:
		return Value.Number;
	case ELuaValueType::String:
		return FCString::Atoi(*Value.String);
	}
	return 0;
}

FLuaValue ULuaBlueprintFunctionLibrary::Conv_IntToLuaValue(int32 Value)
{
	return FLuaValue(Value);
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaGetGlobalTableValue(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FString Key)
{
	FLuaValue ReturnValue;
	ULuaState* LuaState = FLuaMachineModule::Get().GetLuaState(State, (UWorld *)WorldContextObject);
	if (LuaState && LuaState->Table.Contains(Key))
	{
		ReturnValue = LuaState->Table[Key];
	}

	return ReturnValue;
}

void ULuaBlueprintFunctionLibrary::LuaSetGlobalTableValue(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FString Key, FLuaValue Value)
{
	ULuaState* LuaState = FLuaMachineModule::Get().GetLuaState(State, (UWorld *)WorldContextObject);
	if (LuaState && LuaState->Table.Contains(Key))
	{
		LuaState->Table[Key] = Value;
	}
}