// Copyright 2018-2023 - Roberto De Ioris

#include "LuaGlobalNameComponent.h"
#include "LuaBlueprintFunctionLibrary.h"

// Sets default values for this component's properties
ULuaGlobalNameComponent::ULuaGlobalNameComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

void ULuaGlobalNameComponent::OnRegister()
{
	Super::OnRegister();

	if (GetWorld()->IsGameWorld() && !LuaGlobalName.IsEmpty())
	{
		ULuaBlueprintFunctionLibrary::LuaSetGlobal(GetWorld(), LuaState, LuaGlobalName, FLuaValue(GetOwner()));
	}
}

// Called when the game starts
void ULuaGlobalNameComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
}


// Called every frame
void ULuaGlobalNameComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

