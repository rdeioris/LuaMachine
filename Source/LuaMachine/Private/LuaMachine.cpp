// Copyright 2019 - Roberto De Ioris

#include "LuaMachine.h"
#if WITH_EDITOR
#include "Editor/UnrealEd/Public/Editor.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "Runtime/Projects/Public/Interfaces/IPluginManager.h"
#include "SlateCore/Public/Styling/SlateStyleRegistry.h"
#endif

#define LOCTEXT_NAMESPACE "FLuaMachineModule"

void FLuaMachineModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
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

	TMap<TSubclassOf<ULuaState>, ULuaState*> PersistentLuaStates;

	for (TSubclassOf<ULuaState> LuaStateClass : LuaStatesKeys)
	{
		if (LuaStates[LuaStateClass]->bPersistent)
		{
			PersistentLuaStates.Add(LuaStateClass, LuaStates[LuaStateClass]);
		}
		else
		{
			LuaStates[LuaStateClass]->RemoveFromRoot();
		}
	}

	LuaStates = PersistentLuaStates;
	OnRegisteredLuaStatesChanged.Broadcast();
}

ULuaState* FLuaMachineModule::GetLuaState(TSubclassOf<ULuaState> LuaStateClass, UWorld* InWorld, bool bCheckOnly)
{
	if (!LuaStateClass)
		return nullptr;

	if (LuaStateClass == ULuaState::StaticClass())
	{
		UE_LOG(LogLuaMachine, Error, TEXT("attempt to use LuaState Abstract class, please create a child of LuaState"));
		return nullptr;
	}

	if (!LuaStates.Contains(LuaStateClass))
	{
		if (bCheckOnly)
			return nullptr;
		ULuaState* NewLuaState = NewObject<ULuaState>((UObject *)GetTransientPackage(), LuaStateClass);
		LuaStates.Add(LuaStateClass, NewLuaState);
		LuaStates[LuaStateClass]->AddToRoot();
		OnNewLuaState.Broadcast(NewLuaState);
		OnRegisteredLuaStatesChanged.Broadcast();
	}
	return LuaStates[LuaStateClass]->GetLuaState(InWorld);
}

TArray<ULuaState*> FLuaMachineModule::GetRegisteredLuaStates()
{
	TArray<ULuaState*> RegisteredStates;
	for (TPair< TSubclassOf<ULuaState>, ULuaState*>& Pair : LuaStates)
	{
		RegisteredStates.Add(Pair.Value);
	}

	return RegisteredStates;
}

void FLuaMachineModule::UnregisterLuaState(ULuaState* LuaState)
{
	TSubclassOf<ULuaState> FoundLuaStateClass = nullptr;
	for (TPair< TSubclassOf<ULuaState>, ULuaState*>& Pair : LuaStates)
	{
		if (Pair.Value == LuaState)
		{
			FoundLuaStateClass = Pair.Key;
			break;
		}
	}

	if (FoundLuaStateClass)
	{

		LuaStates[FoundLuaStateClass]->RemoveFromRoot();
		LuaStates.Remove(FoundLuaStateClass);
	}

	// trick for waking up on low-level destuctor
	OnRegisteredLuaStatesChanged.Broadcast();
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