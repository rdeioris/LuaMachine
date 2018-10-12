// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "LuaMachine.h"
#if WITH_EDITOR
#include "Editor/UnrealEd/Public/Editor.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#endif

#define LOCTEXT_NAMESPACE "FLuaMachineModule"

void FLuaMachineModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));

	PropertyModule.RegisterCustomPropertyTypeLayout(TEXT("LuaValue"), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FLuaValueCustomization::MakeInstance));
	PropertyModule.RegisterCustomClassLayout(TEXT("LuaCode"), FOnGetDetailCustomizationInstance::CreateStatic(&FLuaCodeCustomization::MakeInstance));

	PropertyModule.NotifyCustomizationModuleChanged();

#if WITH_EDITOR
	FEditorDelegates::BeginPIE.AddRaw(this, &FLuaMachineModule::CleanupLuaStates);
	FEditorDelegates::EndPIE.AddRaw(this, &FLuaMachineModule::CleanupLuaStates);
#endif
}

void FLuaMachineModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

void FLuaMachineModule::CleanupLuaStates(bool bIsSimulating)
{
	TArray<TSubclassOf<ULuaState>> LuaStatesKeys;
	LuaStates.GetKeys(LuaStatesKeys);

	for (TSubclassOf<ULuaState> LuaStateClass : LuaStatesKeys)
	{
		LuaStates[LuaStateClass]->RemoveFromRoot();
		LuaStates[LuaStateClass]->ConditionalBeginDestroy();
	}

	LuaStates.Empty();
}

ULuaState* FLuaMachineModule::GetLuaState(TSubclassOf<ULuaState> LuaStateClass, UWorld* InWorld)
{

	if (!LuaStates.Contains(LuaStateClass))
	{
		LuaStates.Add(LuaStateClass, NewObject<ULuaState>(GetTransientPackage(), LuaStateClass));
		LuaStates[LuaStateClass]->AddToRoot();
	}
	return LuaStates[LuaStateClass]->GetLuaState(InWorld);
}

FLuaMachineModule& FLuaMachineModule::Get()
{
	static FLuaMachineModule* Singleton = nullptr;
	if (Singleton == nullptr)
	{
		Singleton = &FModuleManager::LoadModuleChecked<FLuaMachineModule>("LuaMachine");
	}
	return *Singleton;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FLuaMachineModule, LuaMachine)