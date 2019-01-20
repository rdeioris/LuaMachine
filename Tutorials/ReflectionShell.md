# Implementing a Lua Reflection Shell

The main objective of the LuaMachine project is to safely expose Lua apis to scripters (and modders), so the Unreal Engine reflection system is
completely hidden by default.

If you know what you are doing, you can implement UObject's reflection easily in LuaMachine using C++.

This tutorial will show you how with a bunch of C++ lines you can inspect/modify UObject's at runtime (and obviously make mess eveywhere).

Note that is has been tested with LuaMachine 20190122 and Unreal Engine 4.21 using the ThirdPersonCharacter template.

## Creating the LuaReflectionState

## Getting the list of UProperties

## Getting/Setting Properties

## Syntactic Sugar

## Managing ArrayProperties

## The LuaReflectionComponent

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LuaCode.h"
#include "LuaReflectionComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LUATEST421_API ULuaReflectionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	ULuaReflectionComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ULuaCode* ShellSetupCode;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString ShellName;
};
```

```cpp

#include "LuaReflectionComponent.h"
#include "LuaBlueprintFunctionLibrary.h"
#include "LuaReflectionState.h"

// Sets default values for this component's properties
ULuaReflectionComponent::ULuaReflectionComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void ULuaReflectionComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...

	if (!ShellName.IsEmpty())
	{
		FLuaValue Value = FLuaValue(GetOwner());
		if (ShellSetupCode)
		{
			FLuaValue SetupFunction = ULuaBlueprintFunctionLibrary::LuaRunCodeAsset(GetWorld(), ULuaReflectionState::StaticClass(), ShellSetupCode);
			if (SetupFunction.Type == ELuaValueType::Function)
			{
				TArray<FLuaValue> LuaArgs = { Value };
				Value = ULuaBlueprintFunctionLibrary::LuaValueCall(SetupFunction, LuaArgs);
			}
		}
		ULuaBlueprintFunctionLibrary::LuaSetGlobal(GetWorld(), ULuaReflectionState::StaticClass(), ShellName, Value);
	}
}


// Called every frame
void ULuaReflectionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}
```

## Bonus: calling UObject's methods

## Bonus: binding lua functions to UObject's events

## Bonus: converting USTRUCTs to FLuaValues

## What to do now ?

* try implementing a TMap wrapper

* Add more USTRUCTs conversions
