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
	bWantsInitializeComponent = true;

	// ...

	bLazy = false;
}

// Called when the game starts
void ULuaComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...

}

// Called every frame
void ULuaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void ULuaComponent::InitializeComponent()
{
	if (!bLazy)
		FLuaMachineModule::Get().GetLuaState(LuaState, GetWorld());
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
		if (Pair.Value.Type == ELuaValueType::Function)
		{
			UFunction* Function = GetOwner()->FindFunction(Pair.Value.FunctionName);
			if (Function)
			{
				FLuaUserData* LuaCallContext = (FLuaUserData*)L->NewUserData(sizeof(FLuaUserData));
				LuaCallContext->Type = ELuaValueType::Function;
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

FLuaValue ULuaComponent::LuaCallFunction(FString FunctionName, TArray<FLuaValue> Args)
{
	FLuaValue ReturnValue;

	ULuaState* L = FLuaMachineModule::Get().GetLuaState(LuaState, GetWorld());
	if (!L)
		return ReturnValue;

	// push component pointer as userdata
	L->NewUObject(this);
	SetupMetatable();

	int32 ItemsToPop = L->GetFunctionFromTree(FunctionName);
	if (ItemsToPop <= 0)
	{
		L->Pop(FMath::Abs(ItemsToPop));
		return ReturnValue;
	}

	L->PushValue(-(ItemsToPop+1));
	int NArgs = 1;
	for (FLuaValue& Arg : Args)
	{
		L->FromLuaValue(Arg);
		NArgs++;
	}

	ReturnValue = L->PCall(NArgs);
	if (ReturnValue.Type == ELuaValueType::Error)
	{
		L->LogError(FString::Printf(TEXT("Lua execution error: %s"), *ReturnValue.ErrorMessage));
		ReturnValue.Type = ELuaValueType::Nil;
	}
	// we have the return value and the function has been removed, so we do not need to change ItemsToPop
	L->Pop(ItemsToPop);

	return ReturnValue;
}

