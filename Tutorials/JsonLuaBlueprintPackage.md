```c++

#pragma once

#include "CoreMinimal.h"
#include "LuaBlueprintPackage.h"
#include "JsonLuaBlueprintPackage.generated.h"

/**
 * 
 */
UCLASS()
class RPGLUA_API UJsonLuaBlueprintPackage : public ULuaBlueprintPackage
{
	GENERATED_BODY()

public:

	UJsonLuaBlueprintPackage();

	UFUNCTION()
	FLuaValue ToJson(FLuaValue Value);

	UFUNCTION()
	FLuaValue FromJson(FLuaValue Value);
	
};

```

```c++
#include "JsonLuaBlueprintPackage.h"
#include "LuaBlueprintFunctionLibrary.h"

UJsonLuaBlueprintPackage::UJsonLuaBlueprintPackage()
{
	Table.Add("to_json", FLuaValue::Function(GET_FUNCTION_NAME_CHECKED(UJsonLuaBlueprintPackage, ToJson)));
	Table.Add("from_json", FLuaValue::Function(GET_FUNCTION_NAME_CHECKED(UJsonLuaBlueprintPackage, FromJson)));
}

FLuaValue UJsonLuaBlueprintPackage::FromJson(FLuaValue Value)
{
	FLuaValue ReturnValue;
	ULuaBlueprintFunctionLibrary::LuaValueFromJson(GetWorld(), GetLuaState(), Value.ToString(), ReturnValue);
	return ReturnValue;
}

FLuaValue UJsonLuaBlueprintPackage::ToJson(FLuaValue Value)
{
	return(ULuaBlueprintFunctionLibrary::LuaValueToJson(Value));
}
```
