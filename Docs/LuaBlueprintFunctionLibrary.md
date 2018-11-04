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
