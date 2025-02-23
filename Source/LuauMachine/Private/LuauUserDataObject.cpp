// Copyright 2018-2023 - Roberto De Ioris
// Portions Copyright 2025 - Fusion Point Studios, Inc.

#include "LuauUserDataObject.h"

TSubclassOf<ULuauState> ULuauUserDataObject::GetLuauState() const
{
	ULuauState* LuauState = Cast<ULuauState>(GetOuter());
	if (LuauState)
	{
		return LuauState->GetClass();
	}
	return nullptr;
}

ULuauState* ULuauUserDataObject::GetLuauStateInstance() const
{
	return Cast<ULuauState>(GetOuter());
}

UWorld* ULuauUserDataObject::GetWorld() const
{
	ULuauState* LuauState = Cast<ULuauState>(GetOuter());
	if (LuauState)
	{
		return LuauState->GetWorld();
	}
	return nullptr;
}

TSharedPtr<FLuauSmartReference> ULuauUserDataObject::AddLuauSmartReference(FLuauValue Value)
{
	ULuauState* LuauState = Cast<ULuauState>(GetOuter());
	if (LuauState)
	{
		return LuauState->AddLuauSmartReference(Value);
	}
	return nullptr;
}

void ULuauUserDataObject::RemoveLuauSmartReference(TSharedPtr<FLuauSmartReference> Ref)
{
	if (!Ref.IsValid())
		return;

	ULuauState* LuauState = Cast<ULuauState>(GetOuter());
	if (LuauState)
	{
		return LuauState->RemoveLuauSmartReference(Ref.ToSharedRef());
	}
}

FLuauValue ULuauUserDataObject::LuauGetField(const FString& Name)
{
	ULuauState* LuauState = Cast<ULuauState>(GetOuter());
	if (!LuauState)
		return FLuauValue();

	// push component pointer as userdata
	LuauState->NewUObject(this, nullptr);
	LuauState->SetupAndAssignUserDataMetatable(this, Metatable, nullptr);

	int32 ItemsToPop = LuauState->GetFieldFromTree(Name, false);
	FLuauValue ReturnValue = LuauState->ToLuauValue(-1);

	// we need to remove the return value and the object
	LuauState->Pop(ItemsToPop + 1);

	return ReturnValue;
}

void ULuauUserDataObject::LuauSetField(const FString& Name, FLuauValue Value)
{
	ULuauState* LuauState = Cast<ULuauState>(GetOuter());
	if (!LuauState)
		return;

	// push component pointer as userdata
	LuauState->NewUObject(this, nullptr);
	LuauState->SetupAndAssignUserDataMetatable(this, Metatable, nullptr);

	LuauState->SetFieldFromTree(Name, Value, false, this);

	// remove UObject
	LuauState->Pop();
}

TArray<FString> ULuauUserDataObject::GetObjectUFunctions(bool bOnlyPublic)
{
	TArray<FString> FunctionNames;

	UClass* RelevantClass = GetClass();
	UClass* ParentClass = RelevantClass->GetSuperClass();
	while (ParentClass != ULuauUserDataObject::StaticClass())
	{
		RelevantClass = ParentClass;
		ParentClass = ParentClass->GetSuperClass();
	}

	for (TFieldIterator<UFunction> It(GetClass()); It; ++It)
	{
		UFunction* Function = *It;
		bool bSuccess = true;
		if (bOnlyPublic && !Function->HasAnyFunctionFlags(EFunctionFlags::FUNC_Public))
		{
			bSuccess = false;
		}
		
		if (bSuccess)
		{
			if (Function->GetOuterUClass()->IsChildOf(RelevantClass))
			{
				UFunction* SuperFunction = Function->GetSuperFunction();
				if (SuperFunction && !SuperFunction->GetOuterUClass()->IsChildOf(RelevantClass))
				{
					continue;
				}
				FunctionNames.Add(Function->GetName());
			}
		}
	}

	return FunctionNames;
}

void ULuauUserDataObject::ReceiveLuauUserDataTableInit_Implementation()
{

}

void ULuauUserDataObject::ReceiveLuauGC_Implementation()
{

}

FLuauValue ULuauUserDataObject::ReceiveLuauMetaIndex_Implementation(FLuauValue Key)
{
	return FLuauValue();
}

FLuauValue ULuauUserDataObject::UFunctionToLuauValue(const FString& FunctionName)
{
	FName FunctionFName = FName(*FunctionName);
	UFunction* Function = FindFunction(FunctionFName);
	if (!Function)
	{
		return FLuauValue();
	}

	return FLuauValue::Function(FunctionFName);
}

FLuauValue ULuauUserDataObject::LuauCallFunction(const FString& Name, TArray<FLuauValue> Args, bool bGlobal)
{
	FLuauValue ReturnValue;

	ULuauState* L = GetLuauStateInstance();
	if (!L)
		return ReturnValue;

	// push userdata pointer as userdata
	L->NewUObject(this, nullptr);
	L->SetupAndAssignUserDataMetatable(this, Metatable, nullptr);

	int32 ItemsToPop = L->GetFieldFromTree(Name, bGlobal);

	// first argument (self/actor)
	L->PushValue(-(ItemsToPop + 1));
	int NArgs = 1;
	for (FLuauValue& Arg : Args)
	{
		L->FromLuauValue(Arg);
		NArgs++;
	}

	L->PCall(NArgs, ReturnValue);

	// the return value and the function has been removed, so we do not need to change ItemsToPop
	L->Pop(ItemsToPop + 1);

	return ReturnValue;
}
