// Copyright 2018-2020 - Roberto De Ioris


#include "LuaFunction.h"
#include "LuaBlueprintFunctionLibrary.h"

DEFINE_FUNCTION(ULuaFunction::CallLuaNative)
{
	ULuaFunction* Function = (ULuaFunction*)Stack.CurrentNativeFunction;

	void* LocalStruct = FMemory_Alloca(FMath::Max<int32>(1, Function->GetStructureSize()));
	Function->InitializeStruct(LocalStruct);

	TArray<FLuaValue> LuaArgs;
	// add object ass the first argument
	LuaArgs.Add(FLuaValue(Context));
	
#if ENGINE_MINOR_VERSION >= 25
	for (TFieldIterator<FProperty> ParamsIt(Function); ParamsIt; ++ParamsIt)
#else
	for (TFieldIterator<UProperty> ParamsIt(Function); ParamsIt; ++ParamsIt)
#endif
	{
#if ENGINE_MINOR_VERSION >= 25
		FProperty* Param = *ParamsIt;
#else
		UProperty* Param = *ParamsIt;
#endif
		// skip return value
		if (Param->HasAnyPropertyFlags(CPF_ReturnParm))
		{
			continue;
		}

		void* LocalValue = Param->ContainerPtrToValuePtr<void>(LocalStruct);
#if ENGINE_MINOR_VERSION >= 25
		Stack.StepCompiledIn<FProperty>(LocalValue);
#else
		Stack.StepCompiledIn<UProperty>(LocalValue);
#endif
		bool bSuccess = false;
		FLuaValue LuaArg = Function->LuaState->FromProperty(LocalValue, Param, bSuccess, 0);
		LuaArgs.Add(LuaArg);
	}

	if (Stack.Code)
	{
		checkSlow(*Stack.Code == EX_EndFunctionParms);
		++Stack.Code;
	}

	FLuaValue LuaReturnValue = ULuaBlueprintFunctionLibrary::LuaValueCallIfNotNil(Function->LuaFunction, LuaArgs);

#if ENGINE_MINOR_VERSION >= 25
	FProperty* ReturnProp = Function->GetReturnProperty();
#else
	UProperty* ReturnProp = Function->GetReturnProperty();
#endif

	if (ReturnProp)
	{

	}

	Function->DestroyStruct(LocalStruct);
}