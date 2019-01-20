# Implementing a Lua Reflection Shell

The main objective of the LuaMachine project is to safely expose Lua apis to scripters (and modders), so the Unreal Engine reflection system is
completely hidden by default.

If you know what you are doing, you can implement UObject's reflection easily in LuaMachine using C++.

This tutorial will show you how with a bunch of C++ lines you can inspect/modify UObject's at runtime (and obviously make mess eveywhere).

Note: this has been tested with LuaMachine 20190122 and Unreal Engine 4.21 using the ThirdPersonCharacter template.

## Creating the LuaReflectionState

## Getting the list of UProperties

```cpp
FLuaValue ULuaReflectionState::GetProperties(FLuaValue Object)
{
	if (Object.Type != ELuaValueType::UObject)
		return FLuaValue();

	UStruct* Class = Cast<UStruct>(Object.Object);
	if (!Class)
		Class = Object.Object->GetClass();

	FLuaValue PropertiesArray = CreateLuaTable();
	int32 Index = 0;

	for (TFieldIterator<UProperty> PropsIterator(Class); PropsIterator; ++PropsIterator)
	{
		UProperty* Property = *PropsIterator;
		PropertiesArray.SetFieldByIndex(Index++, FLuaValue(Property->GetName()));
	}

	return PropertiesArray;
}
```

## Getting/Setting Properties

```cpp
FLuaValue ULuaReflectionState::GetProperty(FLuaValue Object, FLuaValue Name, FLuaValue Index)
{
	if (Object.Type != ELuaValueType::UObject)
		return FLuaValue();

	if (Name.Type != ELuaValueType::String)
		return FLuaValue();

	UStruct* Class = Cast<UStruct>(Object.Object);
	if (!Class)
		Class = Object.Object->GetClass();

	UProperty* Property = Class->FindPropertyByName(Name.ToName());
	if (Property)
	{
		bool bSuccess;
		return FromUProperty(Object.Object, Property, bSuccess, Index.ToInteger());
	}

	return FLuaValue();
}

void ULuaReflectionState::SetProperty(FLuaValue Object, FLuaValue Name, FLuaValue Value, FLuaValue Index)
{
	if (Object.Type != ELuaValueType::UObject)
		return;

	if (Name.Type != ELuaValueType::String)
		return;

	UStruct* Class = Cast<UStruct>(Object.Object);
	if (!Class)
		Class = Object.Object->GetClass();

	UProperty* Property = Class->FindPropertyByName(Name.ToName());
	if (Property)
	{
		bool bSuccess;
		ToUProperty(Object.Object, Property, Value, bSuccess, Index.ToInteger());
	}
}
```

## Syntactic Sugar

```lua
local reflection = {}

function reflection.__index(t, key)
  property_type = get_property_type(t.uobject, key)

  if property_type == 'ArrayProperty' then
    local new_table = {uobject=t.uobject, array_key=key}
    setmetatable(new_table, {
      __index = function(array, array_index)
        return wrap_uobject(get_array_property(array.uobject, array.array_key, array_index-1))
      end,
      __newindex = function(array, array_index, array_value)
        return wrap_uobject(set_array_property(array.uobject, array.array_key, array_index-1, array_value))
      end,
      __len = function(array)
         return get_array_property_len(array.uobject, array.array_key)
      end
    })
    return new_table

  elseif not property_type then
    ufunction_ref = get_function(t.uobject, key)
    if ufunction_ref then
      new_ufunction = { uobject=t.uobject, ufunction=ufunction_ref }
      setmetatable(new_ufunction, {
        __call = function(vtable, ...)
          return call_uobject_method(vtable.uobject, vtable.ufunction, arg)
        end
      })
      return new_ufunction
    end
    return nil
  end

  return wrap_uobject(get_property(t.uobject, key))
end

function reflection.__newindex(t, key, value)
  set_property(t.uobject, key, value)
end

function wrap_uobject(uobject)
  if type(uobject) == 'userdata' then
    local new_table = {}
    new_table.uobject = uobject
    setmetatable(new_table, reflection)
    return new_table
  end
  return uobject
end

return wrap_uobject
```

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

```cpp
FLuaValue ULuaReflectionState::CallUObjectMethod(FLuaValue Object, FLuaValue Method, FLuaValue Args)
{
	if (Object.Type != ELuaValueType::UObject)
		return FLuaValue();

	if (Method.Type != ELuaValueType::UObject)
		return FLuaValue();

	UFunction* Function = (UFunction *)Method.Object;

	FScopeCycleCounterUObject ObjectScope(Object.Object);
	FScopeCycleCounterUObject FunctionScope(Function);

	void* Parameters = FMemory_Alloca(Function->ParmsSize);
	FMemory::Memzero(Parameters, Function->ParmsSize);

	// arguments
	if (Args.Type == ELuaValueType::Table)
	{
		int32 NArgs = 1;
		for (TFieldIterator<UProperty> FArgs(Function); FArgs && ((FArgs->PropertyFlags & (CPF_Parm | CPF_ReturnParm)) == CPF_Parm); ++FArgs)
		{
			UProperty *Prop = *FArgs;

			FLuaValue Parameter = Args.GetFieldByIndex(NArgs++);
			bool bSuccess;
			ToUProperty((UObject *)Parameters, Prop, Parameter, bSuccess);
		}
	}

	Object.Object->ProcessEvent(Function, Parameters);

	// get return value
	for (TFieldIterator<UProperty> FArgs(Function); FArgs; ++FArgs)
	{
		UProperty *Prop = *FArgs;
		if (!Prop->HasAnyPropertyFlags(CPF_ReturnParm | CPF_OutParm))
			continue;
		bool bSuccess;
		return FromUProperty((UObject *)Parameters, Prop, bSuccess);
	}

	return FLuaValue();
}
```

## Bonus: binding Lua functions to UObject's events

```cpp
void ULuaReflectionState::BindEvent(FLuaValue Object, FLuaValue Name, FLuaValue Callable)
{
	if (Object.Type != ELuaValueType::UObject)
		return;

	if (Name.Type != ELuaValueType::String)
		return;

	UStruct* Class = Cast<UStruct>(Object.Object);
	if (!Class)
		Class = Object.Object->GetClass();

	UProperty* Property = Class->FindPropertyByName(Name.ToName());
	if (Property)
	{
		UMulticastDelegateProperty* MulticastDelegateProperty = Cast<UMulticastDelegateProperty>(Property);
		if (MulticastDelegateProperty)
		{
			FMulticastScriptDelegate* MulticastDelegate = MulticastDelegateProperty->GetPropertyValuePtr_InContainer(Object.Object);
			ULuaDelegate *LuaDelegate = NewObject<ULuaDelegate>();
			LuaDelegate->SetLuaCallableAndSignature(Callable, MulticastDelegateProperty->SignatureFunction);
			FScriptDelegate Delegate;
			Delegate.BindUFunction(LuaDelegate, "LuaFakeUFunction");
			MulticastDelegate->Add(Delegate);
		}
	}
}

void ULuaDelegate::ProcessEvent(UFunction *Function, void *Parms)
{
	if (!Callable.LuaState)
		return;

	// arguments
	TArray<FLuaValue> Args;
	for (TFieldIterator<UProperty> FArgs(CallableSignature); FArgs && ((FArgs->PropertyFlags & (CPF_Parm | CPF_ReturnParm)) == CPF_Parm); ++FArgs)
	{
		UProperty *Prop = *FArgs;
		bool bSuccess;
		Args.Add(Callable.LuaState->FromUProperty((UObject *)Parms, Prop, bSuccess));
	}

	ULuaBlueprintFunctionLibrary::LuaValueCall(Callable, Args);
}

void ULuaDelegate::LuaFakeUFunction()
{

}
```

## Bonus: converting USTRUCTs to FLuaValues

## What to do now ?

* try implementing a TMap wrapper

* Add more USTRUCTs conversions
