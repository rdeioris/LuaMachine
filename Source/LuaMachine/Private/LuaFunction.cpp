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
	
	for (TFieldIterator<FProperty> ParamsIt(Function); ParamsIt; ++ParamsIt)
	{
		FProperty* Param = *ParamsIt;
		// skip return value
		if (Param->HasAnyPropertyFlags(CPF_ReturnParm))
		{
			continue;
		}

		void* LocalValue = Param->ContainerPtrToValuePtr<void>(LocalStruct);
		Stack.StepCompiledIn<FProperty>(LocalValue);
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

	FProperty* ReturnProp = Function->GetReturnProperty();

	if (ReturnProp)
	{

	}

	Function->DestroyStruct(LocalStruct);
}