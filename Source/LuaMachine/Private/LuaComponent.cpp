// Copyright 2018-2023 - Roberto De Ioris

#include "LuaComponent.h"
#include "LuaMachine.h"
#include "LuaBlueprintFunctionLibrary.h"
#include "GameFramework/Actor.h"


// Sets default values for this component's properties
ULuaComponent::ULuaComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...

	bLazy = false;
	bLogError = false;
	bImplicitSelf = false;
}

void ULuaComponent::OnRegister()
{
	Super::OnRegister();

	if (GetWorld()->IsGameWorld())
	{
		for (const FString& GlobalName : GlobalNames)
		{
			ULuaBlueprintFunctionLibrary::LuaSetGlobal(GetWorld(), LuaState, GlobalName, FLuaValue(this));
		}
	}
}

// Called when the game starts
void ULuaComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!bLazy)
		FLuaMachineModule::Get().GetLuaState(LuaState, GetWorld());

	// ...

}

// Called every frame
void ULuaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

ULuaState* ULuaComponent::LuaComponentGetState()
{
	return FLuaMachineModule::Get().GetLuaState(LuaState, GetWorld());
}

FLuaValue ULuaComponent::LuaGetField(const FString& Name)
{
	FLuaValue ReturnValue;
	ULuaState* L = LuaComponentGetState();
	if (!L)
		return ReturnValue;

	// push component pointer as userdata
	L->NewUObject(this, nullptr);
	L->SetupAndAssignUserDataMetatable(this, Metatable, nullptr);

	int32 ItemsToPop = L->GetFieldFromTree(Name, false);
	ReturnValue = L->ToLuaValue(-1);

	// we need to remove the return value and the object
	L->Pop(ItemsToPop + 1);

	return ReturnValue;
}

void ULuaComponent::LuaSetField(const FString& Name, FLuaValue Value)
{
	ULuaState* L = LuaComponentGetState();
	if (!L)
		return;

	// push component pointer as userdata
	L->NewUObject(this, nullptr);
	L->SetupAndAssignUserDataMetatable(this, Metatable, nullptr);

	L->SetFieldFromTree(Name, Value, false);

	// remove UObject
	L->Pop();

}

FLuaValue ULuaComponent::LuaCallFunction(const FString& Name, TArray<FLuaValue> Args, bool bGlobal)
{
	FLuaValue ReturnValue;

	ULuaState* L = LuaComponentGetState();
	if (!L)
		return ReturnValue;

	// push component pointer as userdata
	L->NewUObject(this, nullptr);
	L->SetupAndAssignUserDataMetatable(this, Metatable, nullptr);

	int32 ItemsToPop = L->GetFieldFromTree(Name, bGlobal);

	// first argument (self/actor)
	L->PushValue(-(ItemsToPop + 1));
	int NArgs = 1;
	for (FLuaValue& Arg : Args)
	{
		L->FromLuaValue(Arg);
		NArgs++;
	}

	if (!L->PCall(NArgs, ReturnValue))
	{
		if (L->InceptionLevel == 0)
		{
			if (bLogError)
				L->LogError(L->LastError);
			OnLuaError.Broadcast(L->LastError);
		}
	}

	// the return value and the function has been removed, so we do not need to change ItemsToPop
	L->Pop(ItemsToPop + 1);

	return ReturnValue;
}

TArray<FLuaValue> ULuaComponent::LuaCallFunctionMulti(FString Name, TArray<FLuaValue> Args, bool bGlobal)
{
	TArray<FLuaValue> ReturnValue;

	ULuaState* L = LuaComponentGetState();
	if (!L)
		return ReturnValue;

	// push component pointer as userdata
	L->NewUObject(this, nullptr);
	L->SetupAndAssignUserDataMetatable(this, Metatable, nullptr);

	int32 ItemsToPop = L->GetFieldFromTree(Name, bGlobal);
	int32 StackTop = L->GetTop();

	// first argument (self/actor)
	L->PushValue(-(ItemsToPop + 1));
	int NArgs = 1;
	for (FLuaValue& Arg : Args)
	{
		L->FromLuaValue(Arg);
		NArgs++;
	}

	FLuaValue LastReturnValue;
	if (!L->PCall(NArgs, LastReturnValue, LUA_MULTRET))
	{
		if (L->InceptionLevel == 0)
		{
			if (bLogError)
				L->LogError(L->LastError);
			OnLuaError.Broadcast(L->LastError);
		}
	}
	else
	{
		int32 NumOfReturnValues = (L->GetTop() - StackTop) + 1;
		if (NumOfReturnValues > 0)
		{
			for (int32 i = -1; i >= -(NumOfReturnValues); i--)
			{
				ReturnValue.Insert(L->ToLuaValue(i), 0);
			}
			L->Pop(NumOfReturnValues - 1);
		}
	}

	// the return value and the function has been removed, so we do not need to change ItemsToPop
	L->Pop(ItemsToPop + 1);

	return ReturnValue;
}

FLuaValue ULuaComponent::LuaCallValue(FLuaValue Value, TArray<FLuaValue> Args)
{
	FLuaValue ReturnValue;

	ULuaState* L = Value.LuaState.Get();
	if (!L)
		return ReturnValue;

	// push function
	L->FromLuaValue(Value);
	// push component pointer as userdata
	L->NewUObject(this, nullptr);
	L->SetupAndAssignUserDataMetatable(this, Metatable, nullptr);

	int NArgs = 1;
	for (FLuaValue& Arg : Args)
	{
		L->FromLuaValue(Arg);
		NArgs++;
	}

	if (!L->PCall(NArgs, ReturnValue))
	{
		if (L->InceptionLevel == 0)
		{
			if (bLogError)
				L->LogError(L->LastError);
			OnLuaError.Broadcast(L->LastError);
		}
	}

	L->Pop();

	return ReturnValue;
}

FLuaValue ULuaComponent::LuaCallValueIfNotNil(FLuaValue Value, TArray<FLuaValue> Args)
{
	FLuaValue ReturnValue;
	if (Value.Type != ELuaValueType::Nil)
		ReturnValue = LuaCallValue(Value, Args);

	return ReturnValue;
}

FLuaValue ULuaComponent::LuaCallTableKey(FLuaValue InTable, FString Key, TArray<FLuaValue> Args)
{
	FLuaValue ReturnValue;

	if (InTable.Type != ELuaValueType::Table)
		return ReturnValue;

	ULuaState* L = InTable.LuaState.Get();
	if (!L)
		return ReturnValue;

	FLuaValue Value = InTable.GetField(Key);
	if (Value.Type == ELuaValueType::Nil)
		return ReturnValue;

	return LuaCallValue(Value, Args);
}

FLuaValue ULuaComponent::LuaCallTableIndex(FLuaValue InTable, int32 Index, TArray<FLuaValue> Args)
{
	FLuaValue ReturnValue;

	if (InTable.Type != ELuaValueType::Table)
		return ReturnValue;

	ULuaState* L = InTable.LuaState.Get();
	if (!L)
		return ReturnValue;

	FLuaValue Value = InTable.GetFieldByIndex(Index);
	if (Value.Type == ELuaValueType::Nil)
		return ReturnValue;

	return LuaCallValue(Value, Args);
}

TArray<FLuaValue> ULuaComponent::LuaCallValueMulti(FLuaValue Value, TArray<FLuaValue> Args)
{
	TArray<FLuaValue> ReturnValue;

	ULuaState* L = Value.LuaState.Get();
	if (!L)
		return ReturnValue;

	// push function
	L->FromLuaValue(Value);
	int32 StackTop = L->GetTop();

	// push component pointer as userdata
	L->NewUObject(this, nullptr);
	L->SetupAndAssignUserDataMetatable(this, Metatable, nullptr);

	int NArgs = 1;
	for (FLuaValue& Arg : Args)
	{
		L->FromLuaValue(Arg);
		NArgs++;
	}

	FLuaValue LastReturnValue;
	if (!L->PCall(NArgs, LastReturnValue, LUA_MULTRET))
	{
		if (L->InceptionLevel == 0)
		{
			if (bLogError)
				L->LogError(L->LastError);
			OnLuaError.Broadcast(L->LastError);
		}
	}
	else
	{
		int32 NumOfReturnValues = (L->GetTop() - StackTop) + 1;
		if (NumOfReturnValues > 0)
		{
			for (int32 i = -1; i >= -(NumOfReturnValues); i--)
			{
				ReturnValue.Insert(L->ToLuaValue(i), 0);
			}
			L->Pop(NumOfReturnValues - 1);
		}
	}

	L->Pop();

	return ReturnValue;
}

TArray<FLuaValue> ULuaComponent::LuaCallValueMultiIfNotNil(FLuaValue Value, TArray<FLuaValue> Args)
{
	TArray<FLuaValue> ReturnValue;
	if (Value.Type != ELuaValueType::Nil)
		ReturnValue = LuaCallValueMulti(Value, Args);

	return ReturnValue;
}

TArray<FLuaValue> ULuaComponent::LuaCallTableKeyMulti(FLuaValue InTable, FString Key, TArray<FLuaValue> Args)
{
	TArray<FLuaValue> ReturnValue;

	if (InTable.Type != ELuaValueType::Table)
		return ReturnValue;

	ULuaState* L = InTable.LuaState.Get();
	if (!L)
		return ReturnValue;

	FLuaValue Value = InTable.GetField(Key);
	if (Value.Type == ELuaValueType::Nil)
		return ReturnValue;

	return LuaCallValueMulti(Value, Args);
}

TArray<FLuaValue> ULuaComponent::LuaCallTableIndexMulti(FLuaValue InTable, int32 Index, TArray<FLuaValue> Args)
{
	TArray<FLuaValue> ReturnValue;

	if (InTable.Type != ELuaValueType::Table)
		return ReturnValue;

	ULuaState* L = InTable.LuaState.Get();
	if (!L)
		return ReturnValue;

	FLuaValue Value = InTable.GetFieldByIndex(Index);
	if (Value.Type == ELuaValueType::Nil)
		return ReturnValue;

	return LuaCallValueMulti(Value, Args);
}

FLuaValue ULuaComponent::ReceiveLuaMetaIndex_Implementation(FLuaValue Key)
{
	return FLuaValue();
}

bool ULuaComponent::ReceiveLuaMetaNewIndex_Implementation(FLuaValue Key, FLuaValue Value)
{
	return false;
}