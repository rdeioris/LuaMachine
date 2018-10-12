// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Editor/PropertyEditor/Public/IDetailCustomization.h"
#include "LuaCode.generated.h"

/**
 * 
 */
UCLASS()
class LUAMACHINE_API ULuaCode : public UDataAsset
{
	GENERATED_BODY()
	
public:

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	FText Code;
	
	
};

class FLuaCodeCustomization : public IDetailCustomization
{
public:
	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FLuaCodeCustomization());
	}
};

