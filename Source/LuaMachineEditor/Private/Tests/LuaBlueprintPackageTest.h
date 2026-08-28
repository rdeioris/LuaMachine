// Copyright 2025 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "LuaBlueprintPackage.h"
#include "LuaBlueprintPackageTest.generated.h"

/**
 * 
 */
// test-only helper, see the note in LuaUnitTestState.h
UCLASS(Transient, Hidden, NotBlueprintable, NotBlueprintType)
class ULuaBlueprintPackageTest : public ULuaBlueprintPackage
{
	GENERATED_BODY()

public:
	ULuaBlueprintPackageTest();

	void Init() override;

	UFUNCTION()
	FLuaValue DoubleNumber(FLuaValue LuaValue);
	
};
