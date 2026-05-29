// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LuaState.h"
#include "LuaStateComponent.generated.h"


UCLASS(BlueprintType, Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), DisplayName = "Lua State")
class LUAMACHINE_API ULuaStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULuaStateComponent();

	UFUNCTION(BlueprintPure, Category = "Reference")
	ULuaState* GetLuaState();

	UFUNCTION(BlueprintCallable, Category = "Reference")
	void DestroyLuaState();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config")
	TSubclassOf<ULuaState> LuaStateClass;

private:
	UPROPERTY()
	ULuaState* LuaState;
	
};
