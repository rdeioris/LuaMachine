// Copyright 2018-2023 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "LuaValue.h"
#include "UObject/NoExportTypes.h"
#include "LuaDelegate.generated.h"

/**
 * 
 */
UCLASS()
class LUAMACHINE_API ULuaDelegate : public UObject
{
	GENERATED_BODY()
	
public:

	void SetupLuaDelegate(UFunction* InSignature, ULuaState* InLuaState, FLuaValue InLuaValue);

	virtual void ProcessEvent(UFunction* Function, void* Parms) override;

	UFUNCTION()
	void LuaDelegateFunction();

private:
	TWeakObjectPtr<ULuaState> LuaState;
	FLuaValue LuaValue;
	UFunction* LuaDelegateSignature;
};
