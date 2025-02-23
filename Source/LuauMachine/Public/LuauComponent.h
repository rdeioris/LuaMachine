// Copyright 2018-2023 - Roberto De Ioris
// Portions Copyright 2025 - Fusion Point Studios, Inc.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LuauState.h"
#include "LuauValue.h"
#include "LuauComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLuauComponentError, FString, Message);


UCLASS(Blueprintable, ClassGroup=(Scripting), meta=(BlueprintSpawnableComponent) )
class LUAUMACHINE_API ULuauComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	ULuauComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Luau")
	TSubclassOf<ULuauState> LuauState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Luau")
	TMap<FString, FLuauValue> Table;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Luau")
	TMap<FString, FLuauValue> Metatable;

	UPROPERTY(EditAnywhere, Category="Luau")
	bool bLazy;

	UPROPERTY(EditAnywhere, Category="Luau")
	bool bLogError;

	UPROPERTY(EditAnywhere, Category = "Luau")
	bool bImplicitSelf;

	UPROPERTY(EditAnywhere, Category = "Luau")
	TArray<FString> GlobalNames;

	UFUNCTION(BlueprintCallable, Category="Luau", meta = (AutoCreateRefTerm = "Args"))
	FLuauValue LuauCallFunction(const FString& Name, TArray<FLuauValue> Args, bool bGlobal);

	UFUNCTION(BlueprintCallable, Category="Luau", meta = (AutoCreateRefTerm = "Args"))
	FLuauValue LuauCallValue(FLuauValue Value, TArray<FLuauValue> Args);

	UFUNCTION(BlueprintCallable, Category="Luau", meta = (AutoCreateRefTerm = "Args"))
	FLuauValue LuauCallValueIfNotNil(FLuauValue Value, TArray<FLuauValue> Args);

	UFUNCTION(BlueprintCallable, Category="Luau", meta = (AutoCreateRefTerm = "Args"))
	FLuauValue LuauCallTableKey(FLuauValue InTable, FString Key, TArray<FLuauValue> Args);

	UFUNCTION(BlueprintCallable, Category="Luau", meta = (AutoCreateRefTerm = "Args"))
	FLuauValue LuauCallTableIndex(FLuauValue InTable, int32 Index, TArray<FLuauValue> Args);

	UFUNCTION(BlueprintCallable, Category="Luau", meta = (AutoCreateRefTerm = "Args"))
	TArray<FLuauValue> LuauCallFunctionMulti(FString Name, TArray<FLuauValue> Args, bool bGlobal);

	UFUNCTION(BlueprintCallable, Category="Luau", meta = (AutoCreateRefTerm = "Args"))
	TArray<FLuauValue> LuauCallValueMulti(FLuauValue Value, TArray<FLuauValue> Args);

	UFUNCTION(BlueprintCallable, Category="Luau", meta = (AutoCreateRefTerm = "Args"))
	TArray<FLuauValue> LuauCallValueMultiIfNotNil(FLuauValue Value, TArray<FLuauValue> Args);

	UFUNCTION(BlueprintCallable, Category="Luau", meta = (AutoCreateRefTerm = "Args"))
	TArray<FLuauValue> LuauCallTableKeyMulti(FLuauValue InTable, FString Key, TArray<FLuauValue> Args);

	UFUNCTION(BlueprintCallable, Category="Luau", meta = (AutoCreateRefTerm = "Args"))
	TArray<FLuauValue> LuauCallTableIndexMulti(FLuauValue InTable, int32 Index, TArray<FLuauValue> Args);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Luau")
	FLuauValue LuauGetField(const FString& Name);

	UFUNCTION(BlueprintCallable, Category="Luau")
	void LuauSetField(const FString& Name, FLuauValue Value);

	UPROPERTY(BlueprintAssignable, Category = "Luau", meta = (DisplayName = "On Luau Error"))
	FLuauComponentError OnLuauError;

	UFUNCTION(BlueprintNativeEvent, Category = "Luau", meta = (DisplayName = "Luau Component Metatable __index"))
	FLuauValue ReceiveLuauMetaIndex(FLuauValue Key);

	UFUNCTION(BlueprintNativeEvent, Category = "Luau", meta = (DisplayName = "Luau Component Metatable __newindex"))
	bool ReceiveLuauMetaNewIndex(FLuauValue Key, FLuauValue Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Luau")
	ULuauState* LuauComponentGetState();

	virtual void OnRegister() override;

};
