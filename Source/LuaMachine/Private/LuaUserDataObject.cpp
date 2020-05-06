// Copyright 2018-2020 - Roberto De Ioris

#include "LuaUserDataObject.h"

TSubclassOf<ULuaState> ULuaUserDataObject::GetLuaState() const
{
	ULuaState* LuaState = Cast<ULuaState>(GetOuter());
	if (LuaState)
	{
		return LuaState->GetClass();
	}
	return nullptr;
}

ULuaState* ULuaUserDataObject::GetLuaStateInstance() const
{
	return Cast<ULuaState>(GetOuter());
}

UWorld* ULuaUserDataObject::GetWorld() const
{
	ULuaState* LuaState = Cast<ULuaState>(GetOuter());
	if (LuaState)
	{
		return LuaState->GetWorld();
	}
	return nullptr;
}

TSharedPtr<FLuaSmartReference> ULuaUserDataObject::AddLuaSmartReference(FLuaValue Value)
{
	ULuaState* LuaState = Cast<ULuaState>(GetOuter());
	if (LuaState)
	{
		return LuaState->AddLuaSmartReference(Value);
	}
	return nullptr;
}

void ULuaUserDataObject::RemoveLuaSmartReference(TSharedPtr<FLuaSmartReference> Ref)
{
	if (!Ref.IsValid())
		return;

	ULuaState* LuaState = Cast<ULuaState>(GetOuter());
	if (LuaState)
	{
		return LuaState->RemoveLuaSmartReference(Ref.ToSharedRef());
	}
}

FLuaValue ULuaUserDataObject::LuaGetField(FString Name)
{
	ULuaState* LuaState = Cast<ULuaState>(GetOuter());
	if (!LuaState)
		return FLuaValue();

	// push component pointer as userdata
	LuaState->NewUObject(this);
	LuaState->SetupAndAssignUserDataMetatable(this, Metatable);

	int32 ItemsToPop = LuaState->GetFieldFromTree(Name, false);
	FLuaValue ReturnValue = LuaState->ToLuaValue(-1);

	// we need to remove the return value and the object
	LuaState->Pop(ItemsToPop + 1);

	return ReturnValue;
}

void ULuaUserDataObject::LuaSetField(FString Name, FLuaValue Value)
{
	ULuaState* LuaState = Cast<ULuaState>(GetOuter());
	if (!LuaState)
		return;

	// push component pointer as userdata
	LuaState->NewUObject(this);
	LuaState->SetupAndAssignUserDataMetatable(this, Metatable);

	LuaState->SetFieldFromTree(Name, Value, false);

	// remove UObject
	LuaState->Pop();
}

TArray<FString> ULuaUserDataObject::GetObjectUFunctions(bool bOnlyPublic)
{
	TArray<FString> FunctionNames;

	for (TFieldIterator<UFunction> It(GetClass()); It; ++It)
	{
		UFunction* Function = *It;
		bool bSuccess = true;
		if (bOnlyPublic && !Function->HasAnyFunctionFlags(EFunctionFlags::FUNC_Public))
		{
			bSuccess = false;
		}
		if (bSuccess)
			FunctionNames.Add(Function->GetName());
	}

	return FunctionNames;
}

void ULuaUserDataObject::ReceiveLuaUserDataTableInit_Implementation()
{

}

void ULuaUserDataObject::ReceiveLuaGC_Implementation()
{

}

FLuaValue ULuaUserDataObject::ReceiveLuaMetaIndex_Implementation(FLuaValue Key)
{
	return FLuaValue();
}

FLuaValue ULuaUserDataObject::UFunctionToLuaValue(FString FunctionName)
{
	FName FunctionFName = FName(*FunctionName);
	UFunction* Function = FindFunction(FunctionFName);
	if (!Function)
		return FLuaValue();

	return FLuaValue::Function(FunctionFName);
}

FLuaValue ULuaUserDataObject::LuaCallFunction(FString Name, TArray<FLuaValue> Args, bool bGlobal)
{
	FLuaValue ReturnValue;

	ULuaState* L = GetLuaStateInstance();
	if (!L)
		return ReturnValue;

	// push userdata pointer as userdata
	L->NewUObject(this);
	L->SetupAndAssignUserDataMetatable(this, Metatable);

	int32 ItemsToPop = L->GetFieldFromTree(Name, bGlobal);

	// first argument (self/actor)
	L->PushValue(-(ItemsToPop + 1));
	int NArgs = 1;
	for (FLuaValue& Arg : Args)
	{
		L->FromLuaValue(Arg);
		NArgs++;
	}

	L->PCall(NArgs, ReturnValue);

	// the return value and the function has been removed, so we do not need to change ItemsToPop
	L->Pop(ItemsToPop + 1);

	return ReturnValue;
}
