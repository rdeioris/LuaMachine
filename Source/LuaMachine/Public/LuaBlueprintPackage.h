// Copyright 2018-2023 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "LuaState.h"
#include "LuaBlueprintPackage.generated.h"

/**
 * 
 */
UCLASS(Abstract, Blueprintable, HideDropdown)
class LUAMACHINE_API ULuaBlueprintPackage : public UObject
{
	GENERATED_BODY()


public:
	UPROPERTY(EditAnywhere, Category = "Lua")
	TMap<FString, FLuaValue> Table;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lua")
	TSubclassOf<ULuaState> GetLuaState() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lua")
	ULuaState* GetLuaStateInstance() const;
	
	virtual UWorld* GetWorld() const override;

	UPROPERTY()
	FLuaValue SelfTable;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lua")
	FLuaValue GetSelfLuaTable() const { return SelfTable; }

	virtual void Init();

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Init"), Category = "Lua")
	void ReceiveInit();
};
