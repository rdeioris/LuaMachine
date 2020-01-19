// Copyright 2019 - Roberto De Ioris

#include "LuaCode.h"
#include "LuaMachine.h"
#include "Serialization/CustomVersion.h"

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

void ULuaCode::PreSave(const ITargetPlatform* TargetPlatform)
{
	Super::PreSave(TargetPlatform);

	for (ULuaState* LuaState : FLuaMachineModule::Get().GetRegisteredLuaStates())
	{
		if (LuaState->LuaCodeAsset == this)
		{
			FLuaMachineModule::Get().UnregisterLuaState(LuaState);
		}
	}
}
