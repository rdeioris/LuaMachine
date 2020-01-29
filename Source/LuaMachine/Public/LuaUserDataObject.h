// Copyright 2018-2020 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "LuaState.h"
#include "LuaUserDataObject.generated.h"

/**
 * 
 */
UCLASS(Abstract, Blueprintable, HideDropdown)
class LUAMACHINE_API ULuaUserDataObject : public UObject
{
	GENERATED_BODY()
	
public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lua")
	TSubclassOf<ULuaState> GetLuaState() const;

	virtual UWorld* GetWorld() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lua")
	TMap<FString, FLuaValue> Table;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lua")
	TMap<FString, FLuaValue> Metatable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lua")
	bool bImplicitSelf;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lua")
	FLuaValue LuaGetField(FString Name);

	UFUNCTION(BlueprintCallable, Category = "Lua")
	void LuaSetField(FString Name, FLuaValue Value);

protected:
	TSharedPtr<FLuaSmartReference> AddLuaSmartReference(FLuaValue Value);
	void RemoveLuaSmartReference(TSharedPtr<FLuaSmartReference> Ref);
};
