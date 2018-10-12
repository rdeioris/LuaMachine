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
	return Value.ToString();
}

UObject* ULuaBlueprintFunctionLibrary::Conv_LuaValueToObject(FLuaValue Value)
{
	if (Value.Type == ELuaValueType::Object)
	{
		return Value.ObjectPath.ResolveObject();
	}
	return nullptr;
}

FLuaValue ULuaBlueprintFunctionLibrary::Conv_ObjectToLuaValue(UObject* Object)
{
	FLuaValue LuaValue;
	LuaValue.Type = ELuaValueType::Object;
	LuaValue.ObjectPath = FSoftObjectPath(Object);
	return LuaValue;
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

FLuaValue ULuaBlueprintFunctionLibrary::LuaCallGlobalFunction(UObject* WorldContextObject, TSubclassOf<ULuaState> LuaState, FString FunctionName, TArray<FLuaValue> Args)
{
	FLuaValue ReturnValue;
	ULuaState* L = FLuaMachineModule::Get().GetLuaState(LuaState, (UWorld*)WorldContextObject);
	if (!L)
		return ReturnValue;

	int32 ItemsToPop = L->GetFunctionFromTree(FunctionName);
	if (ItemsToPop <= 0)
	{
		L->Pop(FMath::Abs(ItemsToPop));
		return ReturnValue;
	}

	int NArgs = 0;
	for (FLuaValue& Arg : Args)
	{
		L->FromLuaValue(Arg);
		NArgs++;
	}

	ReturnValue = L->PCall(NArgs);
	if (ReturnValue.Type == ELuaValueType::Error)
	{
		L->LogError(FString::Printf(TEXT("Lua execution error: %s"), *ReturnValue.ErrorMessage));
		ReturnValue.Type = ELuaValueType::Nil;
	}
	// we have the return value and the function has been removed, so we do not need to change ItemsToPop
	L->Pop(ItemsToPop);

	return ReturnValue;
}
