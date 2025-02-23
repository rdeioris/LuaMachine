// Copyright 2018-2023 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "LuauValue.h"
#include "UObject/NoExportTypes.h"
#include "LuauDelegate.generated.h"

/**
 * 
 */
UCLASS()
class LUAUMACHINE_API ULuauDelegate : public UObject
{
	GENERATED_BODY()
	
public:

	void SetupLuauDelegate(UFunction* InSignature, ULuauState* InLuauState, FLuauValue InLuauValue);

	virtual void ProcessEvent(UFunction* Function, void* Parms) override;

	UFUNCTION()
	void LuauDelegateFunction();

private:
	TWeakObjectPtr<ULuauState> LuauState;
	FLuauValue LuauValue;
	UFunction* LuauDelegateSignature;
};
