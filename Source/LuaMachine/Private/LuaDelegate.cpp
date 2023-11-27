// Copyright 2018-2023 - Roberto De Ioris


#include "LuaDelegate.h"
#include "LuaBlueprintFunctionLibrary.h"

void ULuaDelegate::LuaDelegateFunction()
{
}

void ULuaDelegate::SetupLuaDelegate(UFunction* InSignature, ULuaState* InLuaState, FLuaValue InLuaValue)
{
	LuaDelegateSignature = InSignature;
	LuaState = InLuaState;
	LuaValue = InLuaValue;
}

void ULuaDelegate::ProcessEvent(UFunction* Function, void* Parms)
{
	if (!LuaState.IsValid())
	{
		return;
	}

	TArray<FLuaValue> LuaArgs;
#if  ENGINE_MAJOR_VERSION > 4 ||ENGINE_MINOR_VERSION >= 25
	for (TFieldIterator<FProperty> It(LuaDelegateSignature); (It && (It->PropertyFlags & (CPF_Parm | CPF_ReturnParm)) == CPF_Parm); ++It)
	{
		FProperty* Prop = *It;
#else
	for (TFieldIterator<UProperty> It(LuaDelegateSignature); (It && (It->PropertyFlags & (CPF_Parm | CPF_ReturnParm)) == CPF_Parm); ++It)
	{
		UProperty* Prop = *It;
#endif
		bool bPropSuccess = false;
		LuaArgs.Add(LuaState->FromProperty(Parms, Prop, bPropSuccess, 0));
	}

	ULuaBlueprintFunctionLibrary::LuaGlobalCallValue(LuaState->GetWorld(), LuaState->GetClass(), LuaValue, LuaArgs);
}
