// Copyright 2025 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "LuaState.h"
#include "LuaUnitTestState.generated.h"

/**
 *
 */
// test-only helper: Hidden keeps it out of the editor class pickers.
// NOTE: the UCLASS cannot be wrapped in #if WITH_DEV_AUTOMATION_TESTS -- UHT does
// not evaluate that macro. Living under Private/ keeps it off the public API.
UCLASS(Transient, Hidden, NotBlueprintable, NotBlueprintType)
class ULuaUnitTestState : public ULuaState
{
	GENERATED_BODY()
public:

	ULuaUnitTestState()
	{
		bLogError = true;

		Table.Add("lambda001", FLuaValue::NewLambda([](TArray<FLuaValue> Args) { return FLuaValue("Hello Test"); }));
		Table.Add("lambda002", FLuaValue::NewLambda([this](TArray<FLuaValue> Args) { return Table["lambda001"]; }));
		Table.Add("lambda003", FLuaValue::NewLambda([](TArray<FLuaValue> Args) { return FString("!!!ERROR!!!"); }));
		Table.Add("dummy", FLuaValue::Function(GET_FUNCTION_NAME_CHECKED(ULuaUnitTestState, DummyFunction)));
		Table.Add("return_table", FLuaValue::Function(GET_FUNCTION_NAME_CHECKED(ULuaUnitTestState, ReturnTableFunction)));
	}

	UFUNCTION()
	FLuaValue DummyFunction();

	UFUNCTION()
	FLuaValue ReturnTableFunction();
};
