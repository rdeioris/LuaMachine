// Copyright 2019 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LuaState.h"
#include "LuaValue.h"
#include "LuaComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLuaComponentError, FString, Message);


UCLASS( ClassGroup=(Scripting), meta=(BlueprintSpawnableComponent) )
class LUAMACHINE_API ULuaComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	ULuaComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua")
	TSubclassOf<ULuaState> LuaState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua")
	TMap<FString, FLuaValue> Table;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua")
	TMap<FString, FLuaValue> Metatable;

	UPROPERTY(EditAnywhere, Category="Lua")
	bool bLazy;

	UPROPERTY(EditAnywhere, Category="Lua")
	bool bLogError;

	UFUNCTION(BlueprintCallable, Category="Lua", meta = (AutoCreateRefTerm = "Args"))
	FLuaValue LuaCallFunction(FString Name, TArray<FLuaValue> Args, bool bGlobal);

	UFUNCTION(BlueprintCallable, Category="Lua", meta = (AutoCreateRefTerm = "Args"))
	FLuaValue LuaCallValue(FLuaValue Value, TArray<FLuaValue> Args);

	UFUNCTION(BlueprintCallable, Category="Lua", meta = (AutoCreateRefTerm = "Args"))
	FLuaValue LuaCallValueIfNotNil(FLuaValue Value, TArray<FLuaValue> Args);

	UFUNCTION(BlueprintCallable, Category="Lua", meta = (AutoCreateRefTerm = "Args"))
	FLuaValue LuaCallTableKey(FLuaValue InTable, FString Key, TArray<FLuaValue> Args);

	UFUNCTION(BlueprintCallable, Category="Lua", meta = (AutoCreateRefTerm = "Args"))
	FLuaValue LuaCallTableIndex(FLuaValue InTable, int32 Index, TArray<FLuaValue> Args);

	UFUNCTION(BlueprintCallable, Category="Lua", meta = (AutoCreateRefTerm = "Args"))
	TArray<FLuaValue> LuaCallFunctionMulti(FString Name, TArray<FLuaValue> Args, bool bGlobal);

	UFUNCTION(BlueprintCallable, Category="Lua", meta = (AutoCreateRefTerm = "Args"))
	TArray<FLuaValue> LuaCallValueMulti(FLuaValue Value, TArray<FLuaValue> Args);

	UFUNCTION(BlueprintCallable, Category="Lua", meta = (AutoCreateRefTerm = "Args"))
	TArray<FLuaValue> LuaCallValueMultiIfNotNil(FLuaValue Value, TArray<FLuaValue> Args);

	UFUNCTION(BlueprintCallable, Category="Lua", meta = (AutoCreateRefTerm = "Args"))
	TArray<FLuaValue> LuaCallTableKeyMulti(FLuaValue InTable, FString Key, TArray<FLuaValue> Args);

	UFUNCTION(BlueprintCallable, Category="Lua", meta = (AutoCreateRefTerm = "Args"))
	TArray<FLuaValue> LuaCallTableIndexMulti(FLuaValue InTable, int32 Index, TArray<FLuaValue> Args);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Lua")
	FLuaValue LuaGetField(FString Name);

	UFUNCTION(BlueprintCallable, Category="Lua")
	void LuaSetField(FString Name, FLuaValue Value);

	UPROPERTY(BlueprintAssignable, Category = "Lua", meta = (DisplayName = "On Lua Error"))
	FLuaComponentError OnLuaError;	
};
