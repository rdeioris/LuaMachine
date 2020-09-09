// Copyright 2018-2020 - Roberto De Ioris
// Reimport system by yama2akira (Akira Yamamoto)

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "Developer/AssetTools/Public/AssetTypeCategories.h"
#include "Runtime/Engine/Classes/Engine/Level.h"
#include "EditorReimportHandler.h"
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

	virtual uint32 GetMenuCategories() const override
	{
		return EAssetTypeCategories::Misc;
	}

	virtual UObject* ImportObject(UClass* InClass, UObject* InOuter, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, bool& OutCanceled) override
	{
		UObject* NewAsset = nullptr;
		if (!Filename.IsEmpty())
		{
			CurrentFilename = Filename;
			return FactoryCreateFile(InClass, InOuter, InName, Flags, Filename, Parms, nullptr, OutCanceled);
		}
		else
		{
			return FactoryCreateNew(InClass, InOuter, InName, Flags, nullptr, GWarn, NAME_None);
		}

		if (NewAsset)
		{
			NewAsset->MarkPackageDirty();
			ULevel::LevelDirtiedEvent.Broadcast();
			NewAsset->PostEditChange();
		}

		return NewAsset;
	}

public:
	virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
	virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
	virtual EReimportResult::Type Reimport(UObject* Obj) override;
};
