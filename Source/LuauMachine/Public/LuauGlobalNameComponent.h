// Copyright 2018-2023 - Roberto De Ioris
// Portions Copyright 2025 - Fusion Point Studios, Inc.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LuauState.h"
#include "LuauGlobalNameComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LUAUMACHINE_API ULuauGlobalNameComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	ULuauGlobalNameComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void OnRegister() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Luau")
	TSubclassOf<ULuauState> LuauState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Luau")
	FString LuauGlobalName;
		
};
