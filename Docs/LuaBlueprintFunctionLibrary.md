# LuaBlueprintFunctionLibrary

This is the BlueprintFunctionLibrary for accessing the LuaMachine api from both C++ and Blueprints.

```cpp
#include "LuaMachine/Public/LuaBlueprintFunctionLibrary.h"
```

## FLuaValue LuaCreateString(FString String)

```cpp
UFUNCTION(BlueprintCallable, BlueprintPure)
static FLuaValue LuaCreateString(FString String);
```

Create a new lua string from an Unreal FString

## FLuaValue LuaCreateNumber(float Value)

```cpp
UFUNCTION(BlueprintCallable, BlueprintPure)
static FLuaValue LuaCreateNumber(float Value);
```

Create a new Lua Number from an Unreal float

## FLuaValue LuaCreateInteger(int32 Value)

```cpp
UFUNCTION(BlueprintCallable, BlueprintPure)
static FLuaValue LuaCreateInteger(int32 Value);
```

Create a new Lua Integer from an Unreal int32

## FLuaValue LuaCreateBool(bool bInBool)

```cpp
UFUNCTION(BlueprintCallable, BlueprintPure)
static FLuaValue LuaCreateBool(bool bInBool);
```

Create a new Lua Integer from an Unreal boolean

## FLuaValue LuaCreateTable(UObject* WorldContextObject, TSubclassOf\<ULuaState\> State)
  
```cpp
UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
static FLuaValue LuaCreateTable(UObject* WorldContextObject, TSubclassOf<ULuaState> State);
```

Create a new Lua Table in the specific LuaState. The passed WorldContext will became the CurrentWorld for the LuaState (so you can call GetWorld() on the LuaState itself)

## FLuaValue LuaCreateObject(UObject* InObject)

```cpp
UFUNCTION(BlueprintCallable, BlueprintPure)
static FLuaValue LuaCreateObject(UObject* InObject);
```

Create a new Lua userdata pointing to an Unreal UObject

