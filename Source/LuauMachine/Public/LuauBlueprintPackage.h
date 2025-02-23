// Copyright 2018-2023 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "LuauState.h"
#include "LuauBlueprintPackage.generated.h"

/**
 * 
 */
UCLASS(Abstract, Blueprintable, HideDropdown)
class LUAUMACHINE_API ULuauBlueprintPackage : public UObject
{
	GENERATED_BODY()


public:
	UPROPERTY(EditAnywhere, Category = "Luau")
	TMap<FString, FLuauValue> Table;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Luau")
	TSubclassOf<ULuauState> GetLuauState() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Luau")
	ULuauState* GetLuauStateInstance() const;
	
	virtual UWorld* GetWorld() const override;

	UPROPERTY()
	FLuauValue SelfTable;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Luau")
	FLuauValue GetSelfLuauTable() const { return SelfTable; }

	virtual void Init();

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Init"), Category = "Luau")
	void ReceiveInit();
};
