// Copyright 2018-2023 - Roberto De Ioris
// Reimport system by yama2akira (Akira Yamamoto)

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "Developer/AssetTools/Public/AssetTypeCategories.h"
#include "Runtime/Engine/Classes/Engine/Level.h"
#include "EditorReimportHandler.h"
#include "AssetTypeActions_Base.h"
#include "LuaCodeFactory.generated.h"

/**
 * 
 */
UCLASS()
class LUAMACHINEEDITOR_API ULuaCodeFactory : public UFactory, public FReimportHandler
{
	GENERATED_UCLASS_BODY()

	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext) override;
	virtual UObject* FactoryCreateBinary(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;
	
	virtual FText GetDisplayName() const override
	{
		return FText::FromString("Lua Code");
	}

	virtual uint32 GetMenuCategories() const override;

	virtual UObject* ImportObject(UClass* InClass, UObject* InOuter, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, bool& OutCanceled) override
	{
		if (!Filename.IsEmpty())
		{
			CurrentFilename = Filename;
			return FactoryCreateFile(InClass, InOuter, InName, Flags, Filename, Parms, nullptr, OutCanceled);
		}

		return FactoryCreateNew(InClass, InOuter, InName, Flags, nullptr, GWarn, NAME_None);
	}

public:
	virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
	virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
	virtual EReimportResult::Type Reimport(UObject* Obj) override;
};


/**
 * Add AssetType Actions, so it can show up in Filters, etc.
 */
class FLuaCodeAssetTypeActions : public FAssetTypeActions_Base
{
public:
	FLuaCodeAssetTypeActions( EAssetTypeCategories::Type InAssetCategory );
	// IAssetTypeActions interface
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual UClass* GetSupportedClass() const override;
	//virtual bool HasActions(const TArray<UObject*>& InObjects) const override { return true; }
	//virtual void GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder) override;
	//virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
	virtual uint32 GetCategories() override;
	virtual bool IsImportedAsset() const override;
	virtual void GetResolvedSourceFilePaths( const TArray<UObject*>& TypeAssets, TArray<FString>& OutSourceFilePaths ) const override;
	// End of IAssetTypeActions interface
};
