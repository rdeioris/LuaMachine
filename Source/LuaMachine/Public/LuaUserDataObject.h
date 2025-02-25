// Copyright 2018-2023 - Roberto De Ioris

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

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lua")
	ULuaState* GetLuaStateInstance() const;

	virtual UWorld* GetWorld() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lua")
	TMap<FString, FLuaValue> Table;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lua")
	TMap<FString, FLuaValue> Metatable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lua")
	bool bImplicitSelf;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lua")
	FLuaValue LuaGetField(const FString& Name);

	UFUNCTION(BlueprintCallable, Category = "Lua")
	void LuaSetField(const FString& Name, FLuaValue Value);

	UFUNCTION(BlueprintNativeEvent, Category = "Lua", meta = (DisplayName = "Lua UserData Metatable __gc"))
	void ReceiveLuaGC();

	UFUNCTION(BlueprintNativeEvent, Category = "Lua", meta = (DisplayName = "Lua UserData Metatable __index"))
	FLuaValue ReceiveLuaMetaIndex(FLuaValue Key);

	UFUNCTION(BlueprintNativeEvent, Category = "Lua", meta = (DisplayName = "Lua UserData Table Init"))
	void ReceiveLuaUserDataTableInit();

	UFUNCTION(BlueprintCallable, Category = "Lua", meta = (AutoCreateRefTerm = "Args"))
	FLuaValue LuaCallFunction(const FString& Name, TArray<FLuaValue> Args, bool bGlobal);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lua")
	FLuaValue UFunctionToLuaValue(const FString& FunctionName);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lua")
	TArray<FString> GetObjectUFunctions(bool bOnlyPublic=true);

protected:
	TSharedPtr<FLuaSmartReference> AddLuaSmartReference(FLuaValue Value);
	void RemoveLuaSmartReference(TSharedPtr<FLuaSmartReference> Ref);
};
