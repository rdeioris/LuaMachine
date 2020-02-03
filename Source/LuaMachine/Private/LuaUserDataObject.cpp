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