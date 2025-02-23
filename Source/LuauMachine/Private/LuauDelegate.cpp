// Copyright 2018-2023 - Roberto De Ioris
// Portions Copyright 2025 - Fusion Point Studios, Inc.


#include "LuauDelegate.h"
#include "LuauBlueprintFunctionLibrary.h"

void ULuauDelegate::LuauDelegateFunction()
{
}

void ULuauDelegate::SetupLuauDelegate(UFunction* InSignature, ULuauState* InLuauState, FLuauValue InLuauValue)
{
	LuauDelegateSignature = InSignature;
	LuauState = InLuauState;
	LuauValue = InLuauValue;
}

void ULuauDelegate::ProcessEvent(UFunction* Function, void* Parms)
{
	if (!LuauState.IsValid())
	{
		return;
	}

	TArray<FLuauValue> LuauArgs;
#if  ENGINE_MAJOR_VERSION > 4 ||ENGINE_MINOR_VERSION >= 25
	for (TFieldIterator<FProperty> It(LuauDelegateSignature); (It && (It->PropertyFlags & (CPF_Parm | CPF_ReturnParm)) == CPF_Parm); ++It)
	{
		FProperty* Prop = *It;
#else
	for (TFieldIterator<UProperty> It(LuauDelegateSignature); (It && (It->PropertyFlags & (CPF_Parm | CPF_ReturnParm)) == CPF_Parm); ++It)
	{
		UProperty* Prop = *It;
#endif
		bool bPropSuccess = false;
		LuauArgs.Add(LuauState->FromProperty(Parms, Prop, bPropSuccess, 0));
	}

	ULuauBlueprintFunctionLibrary::LuauGlobalCallValue(LuauState->GetWorld(), LuauState->GetClass(), LuauValue, LuauArgs);
}
