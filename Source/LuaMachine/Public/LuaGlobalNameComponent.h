// Copyright 2018-2023 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LuaState.h"
#include "LuaGlobalNameComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LUAMACHINE_API ULuaGlobalNameComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	ULuaGlobalNameComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void OnRegister() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lua")
	TSubclassOf<ULuaState> LuaState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lua")
	FString LuaGlobalName;
		
};
