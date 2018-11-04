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
