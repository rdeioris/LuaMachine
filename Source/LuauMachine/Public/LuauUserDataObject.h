// Copyright 2018-2023 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "LuauState.h"
#include "LuauUserDataObject.generated.h"

/**
 * 
 */
UCLASS(Abstract, Blueprintable, HideDropdown)
class LUAUMACHINE_API ULuauUserDataObject : public UObject
{
	GENERATED_BODY()
	
public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Luau")
	TSubclassOf<ULuauState> GetLuauState() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Luau")
	ULuauState* GetLuauStateInstance() const;

	virtual UWorld* GetWorld() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Luau")
	TMap<FString, FLuauValue> Table;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Luau")
	TMap<FString, FLuauValue> Metatable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Luau")
	bool bImplicitSelf;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Luau")
	FLuauValue LuauGetField(const FString& Name);

	UFUNCTION(BlueprintCallable, Category = "Luau")
	void LuauSetField(const FString& Name, FLuauValue Value);

	UFUNCTION(BlueprintNativeEvent, Category = "Luau", meta = (DisplayName = "Luau UserData Metatable __gc"))
	void ReceiveLuauGC();

	UFUNCTION(BlueprintNativeEvent, Category = "Luau", meta = (DisplayName = "Luau UserData Metatable __index"))
	FLuauValue ReceiveLuauMetaIndex(FLuauValue Key);

	UFUNCTION(BlueprintNativeEvent, Category = "Luau", meta = (DisplayName = "Luau UserData Table Init"))
	void ReceiveLuauUserDataTableInit();

	UFUNCTION(BlueprintCallable, Category = "Luau", meta = (AutoCreateRefTerm = "Args"))
	FLuauValue LuauCallFunction(const FString& Name, TArray<FLuauValue> Args, bool bGlobal);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Luau")
	FLuauValue UFunctionToLuauValue(const FString& FunctionName);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Luau")
	TArray<FString> GetObjectUFunctions(bool bOnlyPublic=true);

protected:
	TSharedPtr<FLuauSmartReference> AddLuauSmartReference(FLuauValue Value);
	void RemoveLuauSmartReference(TSharedPtr<FLuauSmartReference> Ref);
};
