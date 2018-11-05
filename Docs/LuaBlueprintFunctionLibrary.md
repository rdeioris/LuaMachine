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

## FLuaValue LuaGetGlobal(UObject* WorldContextObject, TSubclassOf\<ULuaState\> State, FString Name)
 
```cpp
UFUNCTION(BlueprintCallable, meta=(WorldContext="WorldContextObject"))
static FLuaValue LuaGetGlobal(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FString Name);
```

Get 'Name' from the Global Table. Name can be in the dotted form to specify subpaths (example: package.path)

## LuaSetGlobal(UObject* WorldContextObject, TSubclassOf\<ULuaState\> State, FString Name, FLuaValue Value)

```cpp
UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
static void LuaSetGlobal(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FString Name, FLuaValue Value);
```

Assign 'Value' to 'Name' in the Global Table. Name can be in the dotted form to specify subpaths (example: package.path)

## FLuaValue LuaTableGetField(FLuaValue Table, FString Key)

```cpp
UFUNCTION(BlueprintCallable, BlueprintPure)
static FLuaValue LuaTableGetField(FLuaValue Table, FString Key);
```

Get a value from a lua table

