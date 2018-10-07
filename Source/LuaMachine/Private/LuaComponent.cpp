// Fill out your copyright notice in the Description page of Project Settings.

#include "LuaComponent.h"
#include "LuaMachine.h"
#include "GameFramework/Actor.h"


// Sets default values for this component's properties
ULuaComponent::ULuaComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...


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

FLuaValue ULuaComponent::LuaCallFunction(FString Function, TArray<FLuaValue> Args)
{
	FLuaValue ReturnValue;

	ULuaState* L = FLuaMachineModule::Get().GetLuaState(LuaState);
	if (!L)
		return ReturnValue;

	// push actor data
	L->NewTable();
	L->NewActor(GetOwner());
	L->SetField(-2, "__unreal_actor");
	// prepare the metatable
	L->NewTable();
	L->PushCFunction(ULuaState::MetaTableFunctionActor__index);
	L->SetField(-2, "__index");
	L->SetMetaTable(-2);

	int32 ItemsToPop = L->GetFunctionFromTree(Function);
	if (ItemsToPop <= 0)
	{
		L->Pop(FMath::Abs(ItemsToPop));
		return ReturnValue;
	}

	L->PushValue(-(ItemsToPop + 1));
	int NArgs = 1;
	for (FLuaValue Arg : Args)
	{
		L->FromLuaValue(Arg);
		NArgs++;
	}

	ReturnValue = L->PCall(NArgs);
	if (ReturnValue.Type == ELuaValueType::Error)
	{
		UE_LOG(LogTemp, Error, TEXT("Lua execution error: %s"), *ReturnValue.String);
		ReturnValue.Type = ELuaValueType::Nil;
	}
	// we have the return value and the function has been removed, so we do not need to change ItemsToPop
	L->Pop(ItemsToPop);

	return ReturnValue;
}

