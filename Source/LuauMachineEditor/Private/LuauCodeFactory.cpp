// Copyright 2018-2023 - Roberto De Ioris
// Reimport system by yama2akira (Akira Yamamoto)
// Portions Copyright 2025 - Fusion Point Studios, Inc.

#include "LuauCodeFactory.h"
#include "LuauCode.h"
#include "EditorFramework/AssetImportData.h"
#include "Misc/FileHelper.h"
#include "LuauMachineEditor.h"

ULuauCodeFactory::ULuauCodeFactory(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {

	Formats.Add(FString("lua;Luau Script"));

	bCreateNew = true;
	bEditAfterNew = true;

	bEditorImport = true;

	SupportedClass = ULuauCode::StaticClass();
}

UObject* ULuauCodeFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	return NewObject<ULuauCode>(InParent, InClass, InName, Flags);
}

UObject* ULuauCodeFactory::FactoryCreateBinary(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	bOutOperationCanceled = false;
	FString Code = ANSI_TO_TCHAR((const char*)Buffer);
	ULuauCode *NewAsset = NewObject<ULuauCode>(InParent, InClass, InName, Flags);
	NewAsset->Code = FText::FromString(Code);

	if (NewAsset->AssetImportData != nullptr) {
		NewAsset->AssetImportData->Update(GetCurrentFilename());
		NewAsset->MarkPackageDirty();
	}
	return NewAsset;
}

uint32 ULuauCodeFactory::GetMenuCategories() const
{
	return FLuauMachineEditorModule::Get().GetAssetCategory();
}

bool ULuauCodeFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	ULuauCode* Asset = Cast<ULuauCode>(Obj);
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

void ULuauCodeFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	ULuauCode* Asset = Cast<ULuauCode>(Obj);
	if (Asset && ensure(NewReimportPaths.Num() == 1))
	{
		Asset->AssetImportData->UpdateFilenameOnly(NewReimportPaths[0]);
	}
}

EReimportResult::Type ULuauCodeFactory::Reimport(UObject* Obj)
{
	ULuauCode* Asset = Cast<ULuauCode>(Obj);
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

FLuauCodeAssetTypeActions::FLuauCodeAssetTypeActions( EAssetTypeCategories::Type InAssetCategory )
{

}

FText FLuauCodeAssetTypeActions::GetName() const
{
	return NSLOCTEXT( "LuauMachine", "LuauCodeAssetActions", "Luau Code" );
}

FColor FLuauCodeAssetTypeActions::GetTypeColor() const
{
	return FColor::Red;
}

UClass* FLuauCodeAssetTypeActions::GetSupportedClass() const
{
	return ULuauCode::StaticClass();
}

uint32 FLuauCodeAssetTypeActions::GetCategories()
{
	return FLuauMachineEditorModule::Get().GetAssetCategory();
}

bool FLuauCodeAssetTypeActions::IsImportedAsset() const
{
	return true;
}

void FLuauCodeAssetTypeActions::GetResolvedSourceFilePaths( const TArray<UObject*>& TypeAssets, TArray<FString>& OutSourceFilePaths ) const
{
	for ( auto& Asset : TypeAssets )
	{
		const auto LuauCode = CastChecked<ULuauCode>( Asset );
		if ( LuauCode->AssetImportData )
		{
			LuauCode->AssetImportData->ExtractFilenames( OutSourceFilePaths );
		}
	}
}

