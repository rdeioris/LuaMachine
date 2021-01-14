# The LuaMachine Reflection API

While 'safety' of scripting api exposure is one of the main objective of LuaMachine, sometimes accessing the reflection api of Unreal Engine 4 is pretty useful (and often a quickway for exposing scripting when you are not worried about giving too much power to users)

Albeit the api is usable from blueprints, it is strongly suggested to go to C++ when defining more advanced states.

## Converting between structs and lua tables

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

## Setting property by name

cpp function (expose it to lua)

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

lua example for setting a delegate:

```lua
function setup(actor)
  set_actor_property(actor, 'OnActorHit', function(me, other, normal, hitresult)
    print('hit with', other)
  end
  )
end
```
