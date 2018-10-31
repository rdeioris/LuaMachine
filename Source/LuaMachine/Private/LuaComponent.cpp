// Fill out your copyright notice in the Description page of Project Settings.

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

FLuaValue ULuaComponent::LuaGetField(FString FieldName)
{
	FLuaValue ReturnValue;
	ULuaState* L = FLuaMachineModule::Get().GetLuaState(LuaState, GetWorld());
	if (!L)
		return ReturnValue;

	// push component pointer as userdata
	L->NewUObject(this);
	SetupMetatable();

	int32 ItemsToPop = L->GetFieldFromTree(FieldName, false);
	ReturnValue = L->ToLuaValue(-1);

	// we need to remove the return value and the object
	L->Pop(ItemsToPop + 1);

	return ReturnValue;
}

void ULuaComponent::LuaSetField(FString FieldName, FLuaValue Value)
{
}

FLuaValue ULuaComponent::LuaCallFunction(FString FunctionName, TArray<FLuaValue> Args, bool bGlobal)
{
	FLuaValue ReturnValue;

	ULuaState* L = FLuaMachineModule::Get().GetLuaState(LuaState, GetWorld());
	if (!L)
		return ReturnValue;

	// push component pointer as userdata
	L->NewUObject(this);
	SetupMetatable();

	int32 ItemsToPop = L->GetFieldFromTree(FunctionName, bGlobal);

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