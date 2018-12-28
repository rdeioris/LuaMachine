// Copyright 2018 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LuaCode.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class LUAMACHINE_API ULuaCode : public UDataAsset
{
	GENERATED_BODY()
	
public:

	ULuaCode();

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	FText Code;
	
	UPROPERTY(EditAnywhere)
	bool bCookAsBytecode;

	UPROPERTY()
	TArray<uint8> ByteCode;

	UPROPERTY()
	bool bCooked;

	virtual void Serialize(FArchive& Ar) override;

	virtual void PreSave(const ITargetPlatform* TargetPlatform) override;
};
