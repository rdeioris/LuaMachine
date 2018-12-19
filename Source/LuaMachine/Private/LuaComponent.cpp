// Copyright 2018 - Roberto De Ioris

#include "LuaComponent.h"
#include "LuaMachine.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY(LogLuaMachine);


// Sets default values for this component's properties
ULuaComponent::ULuaComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...

	bLazy = false;
	bLogError = false;
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

void ULuaComponent::SetupMetatable()
{
	ULuaState* L = FLuaMachineModule::Get().GetLuaState(LuaState, GetWorld());
	if (!L)
		return;

	L->NewTable();
	L->PushCFunction(ULuaState::MetaTableFunctionLuaComponent__index);
	L->SetField(-2, "__index");
	L->PushCFunction(ULuaState::MetaTableFunctionLuaComponent__newindex);
	L->SetField(-2, "__newindex");


	for (TPair<FString, FLuaValue>& Pair : Metatable)
	{
		// first check for UFunction
		if (Pair.Value.Type == ELuaValueType::UFunction)
		{
			UFunction* Function = GetOwner()->FindFunction(Pair.Value.FunctionName);
			if (Function)
			{
				FLuaUserData* LuaCallContext = (FLuaUserData*)L->NewUserData(sizeof(FLuaUserData));
				LuaCallContext->Type = ELuaValueType::UFunction;
				LuaCallContext->Context = GetOwner();
				LuaCallContext->Function = Function;
				L->NewTable();
				L->PushCFunction(ULuaState::MetaTableFunction__call);
				L->SetField(-2, "__call");
				L->SetMetaTable(-2);
			}
			else
			{
				L->PushNil();
			}
		}
		else {
			L->FromLuaValue(Pair.Value);
		}
		L->SetField(-2, TCHAR_TO_UTF8(*Pair.Key));
	}

	L->SetMetaTable(-2);
}

FLuaValue ULuaComponent::LuaGetField(FString Name)
{
	FLuaValue ReturnValue;
	ULuaState* L = FLuaMachineModule::Get().GetLuaState(LuaState, GetWorld());
	if (!L)
		return ReturnValue;

	// push component pointer as userdata
	L->NewUObject(this);
	SetupMetatable();

	int32 ItemsToPop = L->GetFieldFromTree(Name, false);
	ReturnValue = L->ToLuaValue(-1);

	// we need to remove the return value and the object
	L->Pop(ItemsToPop + 1);

	return ReturnValue;
}

void ULuaComponent::LuaSetField(FString Name, FLuaValue Value)
{
	ULuaState* L = FLuaMachineModule::Get().GetLuaState(LuaState, GetWorld());
	if (!L)
		return;

	// push component pointer as userdata
	L->NewUObject(this);
	SetupMetatable();

	L->SetFieldFromTree(Name, Value, false);

	// remove UObject
	L->Pop();
	
}

FLuaValue ULuaComponent::LuaCallFunction(FString Name, TArray<FLuaValue> Args, bool bGlobal)
{
	FLuaValue ReturnValue;

	ULuaState* L = FLuaMachineModule::Get().GetLuaState(LuaState, GetWorld());
	if (!L)
		return ReturnValue;

	// push component pointer as userdata
	L->NewUObject(this);
	SetupMetatable();

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

	ULuaState* L = FLuaMachineModule::Get().GetLuaState(LuaState, GetWorld());
	if (!L)
		return ReturnValue;

	// push component pointer as userdata
	L->NewUObject(this);
	SetupMetatable();

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

	ULuaState* L = Value.LuaState;
	if (!L)
		return ReturnValue;

	L->FromLuaValue(Value);
	// push component pointer as userdata
	L->NewUObject(this);
	SetupMetatable();

	int NArgs = 1;
	for (FLuaValue& Arg : Args)
	{
		L->FromLuaValue(Arg);
		NArgs++;
	}

	L->PCall(NArgs, ReturnValue);

	L->Pop();

	return ReturnValue;
}