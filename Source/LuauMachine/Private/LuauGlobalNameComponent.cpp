// Copyright 2018-2023 - Roberto De Ioris
// Portions Copyright 2025 - Fusion Point Studios, Inc.

#include "LuauGlobalNameComponent.h"
#include "LuauBlueprintFunctionLibrary.h"

// Sets default values for this component's properties
ULuauGlobalNameComponent::ULuauGlobalNameComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

void ULuauGlobalNameComponent::OnRegister()
{
	Super::OnRegister();

	if (GetWorld()->IsGameWorld() && !LuauGlobalName.IsEmpty())
	{
		ULuauBlueprintFunctionLibrary::LuauSetGlobal(GetWorld(), LuauState, LuauGlobalName, FLuauValue(GetOwner()));
	}
}

// Called when the game starts
void ULuauGlobalNameComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
}


// Called every frame
void ULuauGlobalNameComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

