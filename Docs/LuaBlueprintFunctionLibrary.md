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

##  TArray\<FLuaValue\> LuaGlobalCallMulti(UObject* WorldContextObject, TSubclassOf\<ULuaState\> State, FString Name, TArray\<FLuaValue\> Args)

```cpp
UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "Args"))
static TArray<FLuaValue> LuaGlobalCallMulti(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FString Name, TArray<FLuaValue> Args);
```

Call a function by its name from the global table and returns multiple values

## TArray\<FLuaValue\> LuaGlobalCallValueMulti(UObject* WorldContextObject, TSubclassOf\<ULuaState\> State, FLuaValue Value, TArray\<FLuaValue\> Args)

```cpp
UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "Args"))
static TArray<FLuaValue> LuaGlobalCallValueMulti(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FLuaValue Value, TArray<FLuaValue> Args);
```

Call a value in the specified State and returns multiple values

##  TArray\<FLuaValue\> LuaValueCallMulti(FLuaValue Value, TArray\<FLuaValue\> Args)

```cpp
UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Args"))
static TArray<FLuaValue> LuaValueCallMulti(FLuaValue Value, TArray<FLuaValue> Args);
```

Call a value and returns multiple values

## int32 LuaValueLength(FLuaValue Value)

```cpp
UFUNCTION(BlueprintCallable, BlueprintPure)
static int32 LuaValueLength(FLuaValue Value);
```

Get the length of a value (equivalent to the lua '#')

## int32 LuaGetTop(UObject* WorldContextObject, TSubclassOf\<ULuaState\> State)

```cpp
UFUNCTION(BlueprintCallable, BlueprintPure, meta = (WorldContext = "WorldContextObject"))
static int32 LuaGetTop(UObject* WorldContextObject, TSubclassOf<ULuaState> State);
```

Get the stack top of the specified State. Useful for debugging/testing

## FLuaValue LuaRunFile(UObject* WorldContextObject, TSubclassOf\<ULuaState\> State, FString Filename, bool bIgnoreNonExistent)

```cpp
UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
static FLuaValue LuaRunFile(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FString Filename, bool bIgnoreNonExistent);
```

Run the specified file (relative to the Content/ directory) in the specified State.

## FLuaValue LuaRunCodeAsset(UObject* WorldContextObject, TSubclassOf\<ULuaState\> State, ULuaCode* CodeAsset)

```cpp
UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
static FLuaValue LuaRunCodeAsset(UObject* WorldContextObject, TSubclassOf<ULuaState> State, ULuaCode* CodeAsset);
```

Run the specified Code Asset in the specified State.

## int32 LuaGetUsedMemory(UObject* WorldContextObject, TSubclassOf\<ULuaState\> State)

```cpp
UFUNCTION(BlueprintCallable, BlueprintPure, meta = (WorldContext = "WorldContextObject"))
static int32 LuaGetUsedMemory(UObject* WorldContextObject, TSubclassOf<ULuaState> State);
```

Get the used memory (in Kbytes) of the specified Lua State.

## void LuaGCCollect(UObject* WorldContextObject, TSubclassOf\<ULuaState\> State)

```cpp
UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
static void LuaGCCollect(UObject* WorldContextObject, TSubclassOf<ULuaState> State);
```

Triggers Lua GC

## void LuaGCStop(UObject* WorldContextObject, TSubclassOf\<ULuaState\> State)

```cpp
UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
static void LuaGCStop(UObject* WorldContextObject, TSubclassOf<ULuaState> State);
```

Disable automatic execution of the lua GC

## void LuaGCRestart(UObject* WorldContextObject, TSubclassOf\<ULuaState\> State)

```cpp
UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
static void LuaGCRestart(UObject* WorldContextObject, TSubclassOf<ULuaState> State);
```

Re-enable automatic execution of the lua GC

## bool LuaValueIsNil(FLuaValue Value)

```cpp
UFUNCTION(BlueprintCallable, BlueprintPure)
static bool LuaValueIsNil(FLuaValue Value);
```

check if a LuaValue is nil

## bool LuaValueIsTable(FLuaValue Value)

```cpp
UFUNCTION(BlueprintCallable, BlueprintPure)
static bool LuaValueIsTable(FLuaValue Value);
```

check if a LuaValue is a table

## Autocasting

```cpp
UFUNCTION(BlueprintPure, meta=(BlueprintAutocast))
static FString Conv_LuaValueToString(FLuaValue Value);

UFUNCTION(BlueprintPure, meta = (BlueprintAutocast))
static FText Conv_LuaValueToText(FLuaValue Value);

UFUNCTION(BlueprintPure, meta = (BlueprintAutocast))
static UObject* Conv_LuaValueToObject(FLuaValue Value);

UFUNCTION(BlueprintPure, meta = (BlueprintAutocast))
static UClass* Conv_LuaValueToClass(FLuaValue Value);

UFUNCTION(BlueprintPure, meta = (BlueprintAutocast))
static FLuaValue Conv_ObjectToLuaValue(UObject* Object);

UFUNCTION(BlueprintPure, meta = (BlueprintAutocast))
static FLuaValue Conv_FloatToLuaValue(float Value);

UFUNCTION(BlueprintPure, meta = (BlueprintAutocast))
static int32 Conv_LuaValueToInt(FLuaValue Value);

UFUNCTION(BlueprintPure, meta = (BlueprintAutocast))
static float Conv_LuaValueToFloat(FLuaValue Value);

UFUNCTION(BlueprintPure, meta = (BlueprintAutocast))
static FLuaValue Conv_IntToLuaValue(int32 Value);

UFUNCTION(BlueprintPure, meta = (BlueprintAutocast))
static FLuaValue Conv_StringToLuaValue(FString Value);

UFUNCTION(BlueprintPure, meta = (BlueprintAutocast))
static bool Conv_LuaValueToBool(FLuaValue Value);
```
