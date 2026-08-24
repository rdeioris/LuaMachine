// Copyright 2025 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LuaValue.h"
#include "LuaUserDataInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class ULuaUserDataInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 *
 */
class LUAMACHINE_API ILuaUserDataInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Lua")
	FLuaValue LuaMetaMethodIndex(const FString& Key);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Lua")
	bool LuaMetaMethodNewIndex(const FString& Key, FLuaValue Value);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Lua")
	void LuaMetaMethodGC();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Lua")
	FLuaValue LuaMetaMethodToString();
};
