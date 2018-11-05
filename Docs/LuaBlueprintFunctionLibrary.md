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

## FLuaValue LuaTableGetByIndex(FLuaValue Table, int32 Index)

```cpp
UFUNCTION(BlueprintCallable, BlueprintPure)
static FLuaValue LuaTableGetByIndex(FLuaValue Table, int32 Index);
```

Get a value from a lua table/array using an index

## TArray\<FLuaValue\> LuaTableGetKeys(FLuaValue Table)
  
 ```cpp
UFUNCTION(BlueprintCallable, BlueprintPure)
static TArray<FLuaValue> LuaTableGetKeys(FLuaValue Table);
 ```
 
 Get the list of keys in a table
 
## TArray\<FLuaValue\> LuaTableGetValues(FLuaValue Table)

```cpp
UFUNCTION(BlueprintCallable, BlueprintPure)
static TArray<FLuaValue> LuaTableGetValues(FLuaValue Table);
```

Get the list of values in a table

## FLuaValue LuaTableSetField(FLuaValue Table, FString Key, FLuaValue Value)

```cpp
UFUNCTION(BlueprintCallable)
static FLuaValue LuaTableSetField(FLuaValue Table, FString Key, FLuaValue Value);
```

Set the field of a table

##  FLuaValue LuaGlobalCall(UObject* WorldContextObject, TSubclassOf\<ULuaState\> State, FString Name, TArray\<FLuaValue\> Args)

```cpp
UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "Args"))
static FLuaValue LuaGlobalCall(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FString Name, TArray<FLuaValue> Args);
```

Call a function by its name from the global table

## FLuaValue LuaGlobalCallValue(UObject* WorldContextObject, TSubclassOf\<ULuaState\> State, FLuaValue Value, TArray\<FLuaValue\> Args)

```cpp
UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "Args"))
static FLuaValue LuaGlobalCallValue(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FLuaValue Value, TArray<FLuaValue> Args);
```

Call a value in the specified State

##  FLuaValue LuaValueCall(FLuaValue Value, TArray\<FLuaValue\> Args)

```cpp
UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Args"))
static FLuaValue LuaValueCall(FLuaValue Value, TArray<FLuaValue> Args);
```

Call a value

## int32 LuaValueLength(FLuaValue Value)

```cpp
UFUNCTION(BlueprintCallable, BlueprintPure)
static int32 LuaValueLength(FLuaValue Value);
```

Get the length of a value (equivalent to the lua '#')
