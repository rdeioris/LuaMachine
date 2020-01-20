// Copyright 2019 - Roberto De Ioris

#include "LuaCodeFactory.h"
#include "LuaCode.h"


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
	return NewAsset;
}