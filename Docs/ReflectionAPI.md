# The LuaMachine Reflection API

While 'safety' of scripting api exposure is one of the main objective of LuaMachine, sometimes accessing the reflection api of Unreal Engine 4 is pretty useful (and often a quickway for exposing scripting when you are not worried about giving too much power to users)

Albeit the api is usable from blueprints, it is strongly suggested to go to C++ when defining more advanced states.

## Converting between structs and lua tables

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
