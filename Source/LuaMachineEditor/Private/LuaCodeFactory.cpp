// Copyright 2018-2023 - Roberto De Ioris
// Reimport system by yama2akira (Akira Yamamoto)

#include "LuaCodeFactory.h"
#include "LuaCode.h"
#include "EditorFramework/AssetImportData.h"
#include "Misc/FileHelper.h"
#include "LuaMachineEditor.h"

ULuaCodeFactory::ULuaCodeFactory(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {

	Formats.Add(FString("lua;Lua Script"));

	bCreateNew = true;
	bEditAfterNew = true;

	bEditorImport = true;

	SupportedClass = ULuaCode::StaticClass();
}

UObject* ULuaCodeFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	return NewObject<ULuaCode>(InParent, InClass, InName, Flags);
}

UObject* ULuaCodeFactory::FactoryCreateBinary(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	bOutOperationCanceled = false;
	FString Code = ANSI_TO_TCHAR((const char*)Buffer);
	ULuaCode *NewAsset = NewObject<ULuaCode>(InParent, InClass, InName, Flags);
	NewAsset->Code = FText::FromString(Code);

	if (NewAsset->AssetImportData != nullptr) {
		NewAsset->AssetImportData->Update(GetCurrentFilename());
		NewAsset->MarkPackageDirty();
	}
	return NewAsset;
}

uint32 ULuaCodeFactory::GetMenuCategories() const
{
	return FLuaMachineEditorModule::Get().GetAssetCategory();
}

bool ULuaCodeFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	ULuaCode* Asset = Cast<ULuaCode>(Obj);
	if (Asset && Asset->AssetImportData)
	{
		Asset->AssetImportData->ExtractFilenames(OutFilenames);

		for (auto FileName : OutFilenames)
		{
			FString FileContent;
			if (FFileHelper::LoadFileToString(/*out*/ FileContent, *FileName))
			{
				if (Asset->Code.ToString().Compare(FileContent) == 0)
				{
					return false;
				}
			}
		}
		return true;
	}
	return false;
}

void ULuaCodeFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	ULuaCode* Asset = Cast<ULuaCode>(Obj);
	if (Asset && ensure(NewReimportPaths.Num() == 1))
	{
		Asset->AssetImportData->UpdateFilenameOnly(NewReimportPaths[0]);
	}
}

EReimportResult::Type ULuaCodeFactory::Reimport(UObject* Obj)
{
	ULuaCode* Asset = Cast<ULuaCode>(Obj);
	if (!Asset)
	{
		return EReimportResult::Failed;
	}

	const FString Filename = Asset->AssetImportData->GetFirstFilename();
	if (!Filename.Len() || IFileManager::Get().FileSize(*Filename) == INDEX_NONE)
	{
		return EReimportResult::Failed;
	}

	EReimportResult::Type Result = EReimportResult::Failed;
	if (UFactory::StaticImportObject(
		Asset->GetClass(), Asset->GetOuter(),
		*Asset->GetName(), RF_Public | RF_Standalone, *Filename, NULL, this))
	{
		if (Asset->GetOuter())
		{
			Asset->GetOuter()->MarkPackageDirty();
		}
		else
		{
			Asset->MarkPackageDirty();
		}
		return EReimportResult::Succeeded;
	}

	return EReimportResult::Failed;
}

FLuaCodeAssetTypeActions::FLuaCodeAssetTypeActions( EAssetTypeCategories::Type InAssetCategory )
{

}

FText FLuaCodeAssetTypeActions::GetName() const
{
	return NSLOCTEXT( "LuaMachine", "LuaCodeAssetActions", "Lua Code" );
}

FColor FLuaCodeAssetTypeActions::GetTypeColor() const
{
	return FColor::Red;
}

UClass* FLuaCodeAssetTypeActions::GetSupportedClass() const
{
	return ULuaCode::StaticClass();
}

uint32 FLuaCodeAssetTypeActions::GetCategories()
{
	return FLuaMachineEditorModule::Get().GetAssetCategory();
}

bool FLuaCodeAssetTypeActions::IsImportedAsset() const
{
	return true;
}

void FLuaCodeAssetTypeActions::GetResolvedSourceFilePaths( const TArray<UObject*>& TypeAssets, TArray<FString>& OutSourceFilePaths ) const
{
	for ( auto& Asset : TypeAssets )
	{
		const auto LuaCode = CastChecked<ULuaCode>( Asset );
		if ( LuaCode->AssetImportData )
		{
			LuaCode->AssetImportData->ExtractFilenames( OutSourceFilePaths );
		}
	}
}

