// Copyright 2018-2020 - Roberto De Ioris
// Reimport system by yama2akira (Akira Yamamoto)

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#if ENGINE_MAJOR_VERSION > 4
#include "UObject/ObjectSaveContext.h"
#endif
#include "LuaCode.generated.h"

struct LUAMACHINE_API FLuaCodeObjectVersion
{
    enum Type
    {
        // Before any version changes were made
        BeforeCustomVersionWasAdded = 0,

        // Fixed duplication of properties
        FixDuplicationOfProperties,

		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1

    };

    // The GUID for this custom version number
    const static FGuid GUID;

private:
	FLuaCodeObjectVersion() {}
};


/**
 * 
 */
UCLASS(BlueprintType)
class LUAMACHINE_API ULuaCode : public UDataAsset
{
	GENERATED_BODY()
	
public:

	ULuaCode();

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Lua")
	FText Code;
	
	UPROPERTY(EditAnywhere, Category="Lua")
	bool bCookAsBytecode;

	UPROPERTY()
	TArray<uint8> ByteCode;

	UPROPERTY()
	bool bCooked;

	virtual void Serialize(FArchive& Ar) override;

#if ENGINE_MAJOR_VERSION > 4
	virtual void PreSave(FObjectPreSaveContext ObjectSaveContext) override;
#else
	virtual void PreSave(const ITargetPlatform* TargetPlatform) override;
#endif

#if WITH_EDITORONLY_DATA
	virtual void PostInitProperties() override;
	virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;

	UPROPERTY(VisibleAnywhere, Instanced, Category = Reimport)
	class UAssetImportData* AssetImportData;
#endif
};
