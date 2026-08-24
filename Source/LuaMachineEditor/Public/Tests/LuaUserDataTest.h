// Copyright 2025 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "LuaUserDataInterface.h"
#include "LuaUserDataTest.generated.h"

/**
 * Very Dummy UserData implementation using a Map of C++ Functions
 */
UCLASS()
class LUAMACHINEEDITOR_API ULuaUserDataTest : public UObject, public ILuaUserDataInterface
{
	GENERATED_BODY()

public:
	FLuaValue LuaMetaMethodIndex_Implementation(const FString& Key) override;
	bool LuaMetaMethodNewIndex_Implementation(const FString& Key, FLuaValue LuaValue) override;

	TMap<FString, TFunction<FLuaValue()>> LuaUserDataEntries;

	UFUNCTION()
	FLuaValue DummyString() const
	{
		return "Dummy";
	}

};
