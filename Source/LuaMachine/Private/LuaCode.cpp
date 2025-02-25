// Copyright 2018-2023 - Roberto De Ioris
// Reimport system by yama2akira (Akira Yamamoto)

#include "LuaCode.h"
#include "LuaMachine.h"
#include "Serialization/CustomVersion.h"
#include "EditorFramework/AssetImportData.h"

const FGuid FLuaCodeObjectVersion::GUID(0x01C2E96A1, 0xE24436EA, 0x6C69B025, 0x14E7FC3);
FCustomVersionRegistration GRegisterLuaCodeCustomVersion(FLuaCodeObjectVersion::GUID, FLuaCodeObjectVersion::LatestVersion, TEXT("LuaCodeVer"));

ULuaCode::ULuaCode()
{
	bCookAsBytecode = true;
	bCooked = false;
}

void ULuaCode::Serialize(FArchive& Ar)
{
	bool bSkipOriginalCode = false;
	if (Ar.IsCooking() && !Ar.IsLoading())
	{
		if (bCookAsBytecode && !Code.IsEmpty())
		{
			bCooked = true;
			FString ErrorString;
			ByteCode = ULuaState::ToByteCode(Code.ToString(), GetPathName(), ErrorString);
			if (!ErrorString.IsEmpty())
			{
				UE_LOG(LogLuaMachine, Error, TEXT("Unable to generate bytecode: %s"), *ErrorString);
			}
			bSkipOriginalCode = true;
		}
	}
	else if (Ar.IsSaving())
	{
		bCooked = false;
		TArray<uint8> EmptyData;
		ByteCode = EmptyData;
	}

	FText OriginalCode;

	if (bSkipOriginalCode)
	{
		OriginalCode = Code;
		Code = FText::GetEmpty();
	}


	Super::Serialize(Ar);
	Ar.UsingCustomVersion(FLuaCodeObjectVersion::GUID);

	if (Ar.CustomVer(FLuaCodeObjectVersion::GUID) < FLuaCodeObjectVersion::FixDuplicationOfProperties)
	{
		Ar << bCooked;
		Ar << Code;
		Ar << bCookAsBytecode;
		Ar << ByteCode;
	}

	if (bSkipOriginalCode)
	{
		Code = OriginalCode;
	}
}

#if ENGINE_MAJOR_VERSION > 4
void ULuaCode::PreSave(FObjectPreSaveContext ObjectSaveContext)
{
	Super::PreSave(ObjectSaveContext);
#else
void ULuaCode::PreSave(const ITargetPlatform * TargetPlatform)
{
	Super::PreSave(TargetPlatform);
#endif

	for (ULuaState* LuaState : FLuaMachineModule::Get().GetRegisteredLuaStates())
	{
		if (LuaState->LuaCodeAsset == this)
		{
			FLuaMachineModule::Get().UnregisterLuaState(LuaState);
		}
	}
}

#if WITH_EDITORONLY_DATA
void ULuaCode::PostInitProperties()
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
	}
	Super::PostInitProperties();
}

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 4
void ULuaCode::GetAssetRegistryTags(FAssetRegistryTagsContext Context) const
{
	if (AssetImportData)
	{
		Context.AddTag(FAssetRegistryTag(SourceFileTagName(), AssetImportData->GetSourceData().ToJson(), FAssetRegistryTag::TT_Hidden));
	}
	Super::GetAssetRegistryTags(Context);
}
#else
void ULuaCode::GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const
{
	if (AssetImportData)
	{
		OutTags.Add(FAssetRegistryTag(SourceFileTagName(), AssetImportData->GetSourceData().ToJson(), FAssetRegistryTag::TT_Hidden));
	}
	Super::GetAssetRegistryTags(OutTags);
}
#endif

#endif
