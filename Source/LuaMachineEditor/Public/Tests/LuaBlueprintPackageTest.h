// Copyright 2025 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "LuaBlueprintPackage.h"
#include "LuaBlueprintPackageTest.generated.h"

/**
 * 
 */
UCLASS()
class LUAMACHINEEDITOR_API ULuaBlueprintPackageTest : public ULuaBlueprintPackage
{
	GENERATED_BODY()

public:
	ULuaBlueprintPackageTest();

	void Init() override;

	UFUNCTION()
	FLuaValue DoubleNumber(FLuaValue LuaValue);
	
};
