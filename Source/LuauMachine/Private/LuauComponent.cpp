// Copyright 2018-2023 - Roberto De Ioris

#include "LuauComponent.h"
#include "LuauMachine.h"
#include "LuauBlueprintFunctionLibrary.h"
#include "GameFramework/Actor.h"


// Sets default values for this component's properties
ULuauComponent::ULuauComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...

	bLazy = false;
	bLogError = false;
	bImplicitSelf = false;
}

void ULuauComponent::OnRegister()
{
	Super::OnRegister();

	if (GetWorld()->IsGameWorld())
	{
		for (const FString& GlobalName : GlobalNames)
		{
			ULuauBlueprintFunctionLibrary::LuauSetGlobal(GetWorld(), LuauState, GlobalName, FLuauValue(this));
		}
	}
}

// Called when the game starts
void ULuauComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!bLazy)
		FLuauMachineModule::Get().GetLuauState(LuauState, GetWorld());

	// ...

}

// Called every frame
void ULuauComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

ULuauState* ULuauComponent::LuauComponentGetState()
{
	return FLuauMachineModule::Get().GetLuauState(LuauState, GetWorld());
}

FLuauValue ULuauComponent::LuauGetField(const FString& Name)
{
	FLuauValue ReturnValue;
	ULuauState* L = LuauComponentGetState();
	if (!L)
		return ReturnValue;

	// push component pointer as userdata
	L->NewUObject(this, nullptr);
	L->SetupAndAssignUserDataMetatable(this, Metatable, nullptr);

	int32 ItemsToPop = L->GetFieldFromTree(Name, false);
	ReturnValue = L->ToLuauValue(-1);

	// we need to remove the return value and the object
	L->Pop(ItemsToPop + 1);

	return ReturnValue;
}

void ULuauComponent::LuauSetField(const FString& Name, FLuauValue Value)
{
	ULuauState* L = LuauComponentGetState();
	if (!L)
		return;

	// push component pointer as userdata
	L->NewUObject(this, nullptr);
	L->SetupAndAssignUserDataMetatable(this, Metatable, nullptr);

	L->SetFieldFromTree(Name, Value, false);

	// remove UObject
	L->Pop();

}

FLuauValue ULuauComponent::LuauCallFunction(const FString& Name, TArray<FLuauValue> Args, bool bGlobal)
{
	FLuauValue ReturnValue;

	ULuauState* L = LuauComponentGetState();
	if (!L)
		return ReturnValue;

	// push component pointer as userdata
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

	if (!L->PCall(NArgs, ReturnValue))
	{
		if (L->InceptionLevel == 0)
		{
			if (bLogError)
				L->LogError(L->LastError);
			OnLuauError.Broadcast(L->LastError);
		}
	}

	// the return value and the function has been removed, so we do not need to change ItemsToPop
	L->Pop(ItemsToPop + 1);

	return ReturnValue;
}

TArray<FLuauValue> ULuauComponent::LuauCallFunctionMulti(FString Name, TArray<FLuauValue> Args, bool bGlobal)
{
	TArray<FLuauValue> ReturnValue;

	ULuauState* L = LuauComponentGetState();
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
	for (FLuauValue& Arg : Args)
	{
		L->FromLuauValue(Arg);
		NArgs++;
	}

	FLuauValue LastReturnValue;
	if (!L->PCall(NArgs, LastReturnValue, LUA_MULTRET))
	{
		if (L->InceptionLevel == 0)
		{
			if (bLogError)
				L->LogError(L->LastError);
			OnLuauError.Broadcast(L->LastError);
		}
	}
	else
	{
		int32 NumOfReturnValues = (L->GetTop() - StackTop) + 1;
		if (NumOfReturnValues > 0)
		{
			for (int32 i = -1; i >= -(NumOfReturnValues); i--)
			{
				ReturnValue.Insert(L->ToLuauValue(i), 0);
			}
			L->Pop(NumOfReturnValues - 1);
		}
	}

	// the return value and the function has been removed, so we do not need to change ItemsToPop
	L->Pop(ItemsToPop + 1);

	return ReturnValue;
}

FLuauValue ULuauComponent::LuauCallValue(FLuauValue Value, TArray<FLuauValue> Args)
{
	FLuauValue ReturnValue;

	ULuauState* L = Value.LuauState.Get();
	if (!L)
		return ReturnValue;

	// push function
	L->FromLuauValue(Value);
	// push component pointer as userdata
	L->NewUObject(this, nullptr);
	L->SetupAndAssignUserDataMetatable(this, Metatable, nullptr);

	int NArgs = 1;
	for (FLuauValue& Arg : Args)
	{
		L->FromLuauValue(Arg);
		NArgs++;
	}

	if (!L->PCall(NArgs, ReturnValue))
	{
		if (L->InceptionLevel == 0)
		{
			if (bLogError)
				L->LogError(L->LastError);
			OnLuauError.Broadcast(L->LastError);
		}
	}

	L->Pop();

	return ReturnValue;
}

FLuauValue ULuauComponent::LuauCallValueIfNotNil(FLuauValue Value, TArray<FLuauValue> Args)
{
	FLuauValue ReturnValue;
	if (Value.Type != ELuauValueType::Nil)
		ReturnValue = LuauCallValue(Value, Args);

	return ReturnValue;
}

FLuauValue ULuauComponent::LuauCallTableKey(FLuauValue InTable, FString Key, TArray<FLuauValue> Args)
{
	FLuauValue ReturnValue;

	if (InTable.Type != ELuauValueType::Table)
		return ReturnValue;

	ULuauState* L = InTable.LuauState.Get();
	if (!L)
		return ReturnValue;

	FLuauValue Value = InTable.GetField(Key);
	if (Value.Type == ELuauValueType::Nil)
		return ReturnValue;

	return LuauCallValue(Value, Args);
}

FLuauValue ULuauComponent::LuauCallTableIndex(FLuauValue InTable, int32 Index, TArray<FLuauValue> Args)
{
	FLuauValue ReturnValue;

	if (InTable.Type != ELuauValueType::Table)
		return ReturnValue;

	ULuauState* L = InTable.LuauState.Get();
	if (!L)
		return ReturnValue;

	FLuauValue Value = InTable.GetFieldByIndex(Index);
	if (Value.Type == ELuauValueType::Nil)
		return ReturnValue;

	return LuauCallValue(Value, Args);
}

TArray<FLuauValue> ULuauComponent::LuauCallValueMulti(FLuauValue Value, TArray<FLuauValue> Args)
{
	TArray<FLuauValue> ReturnValue;

	ULuauState* L = Value.LuauState.Get();
	if (!L)
		return ReturnValue;

	// push function
	L->FromLuauValue(Value);
	int32 StackTop = L->GetTop();

	// push component pointer as userdata
	L->NewUObject(this, nullptr);
	L->SetupAndAssignUserDataMetatable(this, Metatable, nullptr);

	int NArgs = 1;
	for (FLuauValue& Arg : Args)
	{
		L->FromLuauValue(Arg);
		NArgs++;
	}

	FLuauValue LastReturnValue;
	if (!L->PCall(NArgs, LastReturnValue, LUA_MULTRET))
	{
		if (L->InceptionLevel == 0)
		{
			if (bLogError)
				L->LogError(L->LastError);
			OnLuauError.Broadcast(L->LastError);
		}
	}
	else
	{
		int32 NumOfReturnValues = (L->GetTop() - StackTop) + 1;
		if (NumOfReturnValues > 0)
		{
			for (int32 i = -1; i >= -(NumOfReturnValues); i--)
			{
				ReturnValue.Insert(L->ToLuauValue(i), 0);
			}
			L->Pop(NumOfReturnValues - 1);
		}
	}

	L->Pop();

	return ReturnValue;
}

TArray<FLuauValue> ULuauComponent::LuauCallValueMultiIfNotNil(FLuauValue Value, TArray<FLuauValue> Args)
{
	TArray<FLuauValue> ReturnValue;
	if (Value.Type != ELuauValueType::Nil)
		ReturnValue = LuauCallValueMulti(Value, Args);

	return ReturnValue;
}

TArray<FLuauValue> ULuauComponent::LuauCallTableKeyMulti(FLuauValue InTable, FString Key, TArray<FLuauValue> Args)
{
	TArray<FLuauValue> ReturnValue;

	if (InTable.Type != ELuauValueType::Table)
		return ReturnValue;

	ULuauState* L = InTable.LuauState.Get();
	if (!L)
		return ReturnValue;

	FLuauValue Value = InTable.GetField(Key);
	if (Value.Type == ELuauValueType::Nil)
		return ReturnValue;

	return LuauCallValueMulti(Value, Args);
}

TArray<FLuauValue> ULuauComponent::LuauCallTableIndexMulti(FLuauValue InTable, int32 Index, TArray<FLuauValue> Args)
{
	TArray<FLuauValue> ReturnValue;

	if (InTable.Type != ELuauValueType::Table)
		return ReturnValue;

	ULuauState* L = InTable.LuauState.Get();
	if (!L)
		return ReturnValue;

	FLuauValue Value = InTable.GetFieldByIndex(Index);
	if (Value.Type == ELuauValueType::Nil)
		return ReturnValue;

	return LuauCallValueMulti(Value, Args);
}

FLuauValue ULuauComponent::ReceiveLuauMetaIndex_Implementation(FLuauValue Key)
{
	return FLuauValue();
}

bool ULuauComponent::ReceiveLuauMetaNewIndex_Implementation(FLuauValue Key, FLuauValue Value)
{
	return false;
}