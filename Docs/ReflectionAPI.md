# The LuaMachine Reflection API

While 'safety' of scripting api exposure is one of the main objective of LuaMachine, sometimes accessing the reflection api of Unreal Engine is pretty useful (and often a quick way for exposing scripting when you are not worried about giving too much power to users)

Albeit the api is usable from blueprints, it is strongly suggested to go to C++ when defining more advanced states.

## Converting between structs and Lua tables

The reflection API is pretty big, so before going into a more-or-less-useful implementation of Lua Reflection, we will cover some low-level details.

The first one is the automatic conversion between USTRUCTs and lua tables. The following 3 template functions allows easy conversion from any USTRUCT to an FLuaValue and the opposite.

```cpp
template<class T>
FLuaValue StructToLuaValue(T& InStruct);

template<class T>
T LuaValueToStruct(FLuaValue& LuaValue);

template<class T>
T LuaValueToBaseStruct(FLuaValue& LuaValue)
```

Let's implement a SetActorLocation() wrapper for a LuaState:

```cpp
#include "StructManagerLuaState.h"
#include "GameFramework/Actor.h"

UStructManagerLuaState::UStructManagerLuaState()
{
	Table.Add("set_actor_location", FLuaValue::Function(GET_FUNCTION_NAME_CHECKED(UStructManagerLuaState, SetActorLocation)));
}

FLuaValue UStructManagerLuaState::SetActorLocation(FLuaValue Actor, FLuaValue Location)
{
	AActor* ActorObject = Cast<AActor>(Actor.Object);
	if (ActorObject)
	{
		FHitResult Hit;
		// FVector is a base type (like FRotator, FQuat, ...) , so we need to use LuaValueToBaseStruct instead of LuaValueToStruct
		ActorObject->SetActorLocation(LuaValueToBaseStruct<FVector>(Location), true, &Hit);
		// convert the FHitResult struct to a lua table
		return StructToLuaValue(Hit);
	}
	// nil
	return FLuaValue();
}
```

## Getting/Setting properties by name

The following c++/blueprint functions allow to access the Unreal properties using the reflection system:

```cpp
UFUNCTION(BlueprintCallable, Category = "Lua")
FLuaValue GetLuaValueFromProperty(UObject* InObject, const FString& PropertyName);

UFUNCTION(BlueprintCallable, Category = "Lua")
bool SetPropertyFromLuaValue(UObject* InObject, const FString& PropertyName, FLuaValue Value);
```

Properties Values are always converted to FLuaValue.

This is a simple (and pretty useless) example:

```cpp
void UStructManagerLuaState::SetActorProperty(FLuaValue Actor, FLuaValue Property, FLuaValue Value)
{
	AActor* ActorObject = Cast<AActor>(Actor.Object);
	if (ActorObject)
	{
		SetPropertyFromLuaValue(ActorObject, Property.ToString(), Value);
	}
}
```

Delegates are automatically managed as lua functions:

lua example for setting a delegate (assuming the previous defined UStructManagerLuaState::SetActorProperty has been exposed as 'set_actor_property') :

```lua
function setup(actor)
  set_actor_property(actor, 'OnActorHit', function(me, other, normal, hitresult)
    print('hit with', other)
  end
  )
end
```

## Implementing a LuaState that automatically exposes everything to the Lua VM

This is probably the reason you are reading this page ;)

In the following lines i will show you a 'simple' LuaState that automatically uses the Unreal reflection to expose UObject, UFunction, Properties, Structs, Delegates directly to lua without additional steps (in addition to assigning a lua name/variable to them when required).

Let's start by defining the header (LuaReflectionState.h):

```cpp
#pragma once

#include "CoreMinimal.h"
#include "LuaState.h"
#include "LuaReflectionState.generated.h"

UCLASS()
class ULuaReflectionState : public ULuaState
{
	GENERATED_BODY()

public:
	ULuaReflectionState();

protected:
	void LuaStateInit() override;

	// __index(object, key) -> returning 1 value
	LUACFUNCTION(ULuaReflectionState, MetaMethodIndex, 1, 2);

	// __newindex(object, key, value)
	LUACFUNCTION(ULuaReflectionState, MetaMethodNewIndex, 0, 3);

	// __eq(object1, object2) -> returning bool
	LUACFUNCTION(ULuaReflectionState, MetaMethodEq, 1, 2);

	// __string(object) -> returning string
	LUACFUNCTION(ULuaReflectionState, MetaMethodToString, 1, 1);
};
```

The new (and most important) stuff here is the usage of the LUACFUNCTION definition.

Here we are defining 4 metamethods to be called whenever an Unreal object is passed around in the lua state. UObjects are mapped to userdata, so this method defines what to do when using them. As an example, if the 'mannequin' lua variable is mapped to a Character, we want to make it jump by calling mannequin.Jump(). So our system needs to resolve Jump as a function and call it.

We cannot use plain UFunctions for those metamethods (as they are userdata under the hood), so we need to code them using lower level LUA c api. The LUACFUNCTION wraps those low-level functions in Unreal C++ api-friendly ones.

Now let's define the cpp part (LuaReflectionState.cpp):

```cpp
#include "LuaReflectionState.h"
#include "LuaBlueprintFunctionLibrary.h"

ULuaReflectionState::ULuaReflectionState()
{
	// allow to call native UFunctions with implicit FLuaValue conversions
	bRawLuaFunctionCall = true;
}

void ULuaReflectionState::LuaStateInit()
{
	UserDataMetaTable = CreateLuaTable();
	UserDataMetaTable.SetField("__index", ULuaReflectionState::MetaMethodIndex_C);
	UserDataMetaTable.SetField("__newindex", ULuaReflectionState::MetaMethodNewIndex_C);
	UserDataMetaTable.SetField("__eq", ULuaReflectionState::MetaMethodEq_C);
	UserDataMetaTable.SetField("__tostring", ULuaReflectionState::MetaMethodToString_C);
}

TArray<FLuaValue> ULuaReflectionState::MetaMethodIndex(TArray<FLuaValue> LuaArgs)
{
	TArray<FLuaValue> ReturnValues;

	UObject* Object = LuaArgs[0].Object;
	FString Key = LuaArgs[1].ToString();

	// skip nullptr and classes
	if (!Object || Object->IsA<UClass>())
	{
		return ReturnValues;
	}

	ELuaReflectionType ReflectionType = ELuaReflectionType::Unknown;
	ULuaBlueprintFunctionLibrary::GetLuaReflectionType(Object, Key, ReflectionType);

	if (ReflectionType == ELuaReflectionType::Property)
	{
		ReturnValues.Add(GetLuaValueFromProperty(Object, Key));
	}
	else if (ReflectionType == ELuaReflectionType::Function)
	{
		ReturnValues.Add(FLuaValue::FunctionOfObject(Object, FName(Key)));
	}

	return ReturnValues;
}

TArray<FLuaValue> ULuaReflectionState::MetaMethodNewIndex(TArray<FLuaValue> LuaArgs)
{
	TArray<FLuaValue> ReturnValues;

	UObject* Object = LuaArgs[0].Object;
	FString Key = LuaArgs[1].ToString();
	FLuaValue Value = LuaArgs[2];

	// skip nullptr and classes
	if (!Object || Object->IsA<UClass>())
	{
		return ReturnValues;
	}

	ELuaReflectionType ReflectionType = ELuaReflectionType::Unknown;
	ULuaBlueprintFunctionLibrary::GetLuaReflectionType(Object, Key, ReflectionType);

	if (ReflectionType == ELuaReflectionType::Property)
	{
		SetPropertyFromLuaValue(Object, Key, Value);
	}

	return ReturnValues;
}

TArray<FLuaValue> ULuaReflectionState::MetaMethodEq(TArray<FLuaValue> LuaArgs)
{
	TArray<FLuaValue> ReturnValues;

	UObject* Object = LuaArgs[0].Object;
	UObject* OtherObject = LuaArgs[1].Object;

	if (!Object || !OtherObject)
	{
		ReturnValues.Add(FLuaValue(false));
	}
	else
	{
		ReturnValues.Add(FLuaValue(Object == OtherObject));
	}

	return ReturnValues;
}

TArray<FLuaValue> ULuaReflectionState::MetaMethodToString(TArray<FLuaValue> LuaArgs)
{
	TArray<FLuaValue> ReturnValues;

	UObject* Object = LuaArgs[0].Object;

	// skip nullptr and classes
	if (!Object || Object->IsA<UClass>())
	{
		return ReturnValues;
	}

	ReturnValues.Add(Object->GetFullName());

	return ReturnValues;
}
```

We are now ready for building a simple logic in lua (put this in a script or a LuaCodeAsset):

```lua
mannequin.OnActorBeginOverlap = function(me, other)
  print(tostring(me) .. ' colliding with ' .. tostring(other))
  other.K2_AddActorLocalRotation({Yaw=10, Pitch=0, Roll=0})
end

mannequin.Jump()
```

Here we have a script that assumes a character can overlap with a (movable) object that will be rotated of 10 degrees on overlapping.

You can run this script in the LuaReflectionState using a blueprint. The only missing part is assigning your character to the lua 'mannequin' variable. For doing this automatically just add to the character a LuaGlobalNameComponent on which you can specify the state and the name.

Notes:

* You do not need to attach LuaComponents to actors (everything is automatic)
* You can attach multiple LuaGlobalNameComponents on the same actor (allowing it to be available on multiple states or with multiple names)
* The LuaGlobalNameComponent is super easy, just give it a look to adapt it to more complex scenario
* The LuaReflectionState class is not part of the official sources to encourage users/developers to implement their own solutions (like hiding dangerous methods or exposing subsets of them)
