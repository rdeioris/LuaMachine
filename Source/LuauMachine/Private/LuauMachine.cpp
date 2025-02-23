// Copyright 2018-2023 - Roberto De Ioris

#include "LuauMachine.h"
#include "LuauBlueprintFunctionLibrary.h"
#if WITH_EDITOR
#include "Editor/UnrealEd/Public/Editor.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "Runtime/Projects/Public/Interfaces/IPluginManager.h"
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 3
#include "Styling/SlateStyleRegistry.h"
#else
#include "SlateCore/Public/Styling/SlateStyleRegistry.h"
#endif
#endif

#define LOCTEXT_NAMESPACE "FLuauMachineModule"

void FLuauMachineModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
#if WITH_EDITOR
	FEditorDelegates::BeginPIE.AddRaw(this, &FLuauMachineModule::CleanupLuauStates);
	FEditorDelegates::EndPIE.AddRaw(this, &FLuauMachineModule::CleanupLuauStates);
#endif

	// streaming level hooks
	FWorldDelegates::LevelAddedToWorld.AddRaw(this, &FLuauMachineModule::LuauLevelAddedToWorld);
	FWorldDelegates::LevelRemovedFromWorld.AddRaw(this, &FLuauMachineModule::LuauLevelRemovedFromWorld);

}

void FLuauMachineModule::LuauLevelAddedToWorld(ULevel* Level, UWorld* World)
{
	for (ULuauState* LuauState : GetRegisteredLuauStates())
	{
#if ENGINE_MAJOR_VERSION > 4
		if (LuauState->IsValidLowLevel() && !IsValid(LuauState))
#else
		if (LuauState->IsValidLowLevel() && !LuauState->IsPendingKill())
#endif
		{
			LuauState->ReceiveLuauLevelAddedToWorld(Level, World);
		}
	}
}

void FLuauMachineModule::LuauLevelRemovedFromWorld(ULevel* Level, UWorld* World)
{
	for (ULuauState* LuauState : GetRegisteredLuauStates())
	{
#if ENGINE_MAJOR_VERSION > 4
		if (LuauState->IsValidLowLevel() && !IsValid(LuauState))
#else
		if (LuauState->IsValidLowLevel() && !LuauState->IsPendingKill())
#endif
		{
			LuauState->ReceiveLuauLevelRemovedFromWorld(Level, World);
		}
	}
}

void FLuauMachineModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

void FLuauMachineModule::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObjects(LuauStates);
}

void FLuauMachineModule::CleanupLuauStates(bool bIsSimulating)
{
	TArray<TSubclassOf<ULuauState>> LuauStatesKeys;
	LuauStates.GetKeys(LuauStatesKeys);

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 4
	TMap<TSubclassOf<ULuauState>, TObjectPtr<ULuauState>> PersistentLuauStates;
#else
	TMap<TSubclassOf<ULuauState>, ULuauState*> PersistentLuauStates;
#endif

	for (TSubclassOf<ULuauState> LuauStateClass : LuauStatesKeys)
	{
		if (LuauStates[LuauStateClass]->bPersistent)
		{
			PersistentLuauStates.Add(LuauStateClass, LuauStates[LuauStateClass]);
		}
		else
		{
			if (FLuauCommandExecutor* LuauConsole = LuauStates[LuauStateClass]->GetLuauConsole())
			{
				IModularFeatures::Get().UnregisterModularFeature(IConsoleCommandExecutor::ModularFeatureName(), LuauConsole);
			}
		}
	}

	LuauStates = PersistentLuauStates;
	OnRegisteredLuauStatesChanged.Broadcast();
}

ULuauState* FLuauMachineModule::GetLuauState(TSubclassOf<ULuauState> LuauStateClass, UWorld* InWorld, bool bCheckOnly)
{
	if (!LuauStateClass)
	{
		return nullptr;
	}

	if (LuauStateClass == ULuauState::StaticClass())
	{
		UE_LOG(LogLuauMachine, Error, TEXT("attempt to use LuauState Abstract class, please create a child of LuauState"));
		return nullptr;
	}

	if (!LuauStates.Contains(LuauStateClass))
	{
		if (bCheckOnly)
		{
			return nullptr;
		}
		ULuauState* NewLuauState = NewObject<ULuauState>((UObject*)GetTransientPackage(), LuauStateClass);
		LuauStates.Add(LuauStateClass, NewLuauState);
		OnNewLuauState.Broadcast(NewLuauState);
		OnRegisteredLuauStatesChanged.Broadcast();
	}

	return LuauStates[LuauStateClass]->GetLuauState(InWorld);
}

TArray<ULuauState*> FLuauMachineModule::GetRegisteredLuauStates()
{
	TArray<ULuauState*> RegisteredStates;
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 4
	for (TPair< TSubclassOf<ULuauState>, TObjectPtr<ULuauState>>& Pair : LuauStates)
#else
	for (TPair< TSubclassOf<ULuauState>, ULuauState*>& Pair : LuauStates)
#endif
	{
		RegisteredStates.Add(Pair.Value);
	}

	return RegisteredStates;
}

void FLuauMachineModule::UnregisterLuauState(ULuauState* LuauState)
{
	TSubclassOf<ULuauState> FoundLuauStateClass = nullptr;
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 4
	for (TPair< TSubclassOf<ULuauState>, TObjectPtr<ULuauState>>& Pair : LuauStates)
#else
	for (TPair< TSubclassOf<ULuauState>, ULuauState*>& Pair : LuauStates)
#endif
	{
		if (Pair.Value == LuauState)
		{
			FoundLuauStateClass = Pair.Key;
			break;
		}
	}

	if (FoundLuauStateClass)
	{
		LuauStates.Remove(FoundLuauStateClass);
	}

	// trick for waking up on low-level destructor
	OnRegisteredLuauStatesChanged.Broadcast();
}

void FLuauMachineModule::RegisterLuauConsoleCommand(const FString& CommandName, const FLuauValue& LuauConsoleCommand)
{
	// first check in lua console commands
	UnregisterLuauConsoleCommand(CommandName);
	if (IConsoleManager::Get().FindConsoleObject(*CommandName))
	{
		UE_LOG(LogLuauMachine, Error, TEXT("Luau Console Command Name \"%s\" is already in use."), *CommandName);
		return;
	}

	if (IConsoleManager::Get().RegisterConsoleCommand(*CommandName, *CommandName, FConsoleCommandWithArgsDelegate::CreateRaw(this, &FLuauMachineModule::RunLuauConsoleCommand, LuauConsoleCommand)))
	{
		LuauConsoleCommands.Add(CommandName);
	}
}

void FLuauMachineModule::UnregisterLuauConsoleCommand(const FString& CommandName)
{
	if (LuauConsoleCommands.Contains(CommandName))
	{
		IConsoleManager::Get().UnregisterConsoleObject(*CommandName);
		LuauConsoleCommands.Remove(CommandName);
	}
}

void FLuauMachineModule::RunLuauConsoleCommand(const TArray<FString>& Args, FLuauValue LuauConsoleCommand)
{
	TArray<FLuauValue> LuauArgs;
	for (FString Arg : Args)
	{
		LuauArgs.Add(FLuauValue(Arg));
	}
	ULuauBlueprintFunctionLibrary::LuauValueCallIfNotNil(LuauConsoleCommand, LuauArgs);
}

FLuauMachineModule& FLuauMachineModule::Get()
{
	static FLuauMachineModule* Singleton = nullptr;
	if (Singleton == nullptr)
	{
		Singleton = &FModuleManager::LoadModuleChecked<FLuauMachineModule>("LuauMachine");
	}
	return *Singleton;
}

bool FLuauMachineModule::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	if (FParse::Command(&Cmd, TEXT("luaspawn")))
	{
		if (!*Cmd)
		{
			UE_LOG(LogLuauMachine, Error, TEXT("please specify a valid LuauState path."));
			return false;
		}

		UObject* RequestedObject = LoadObject<UObject>(nullptr, Cmd);
		if (RequestedObject)
		{
			bool bLoaded = false;
			UClass* ObjectAsClass = Cast<UClass>(RequestedObject);
			if (ObjectAsClass && ObjectAsClass->IsChildOf<ULuauState>())
			{
				bLoaded = true;
			}

			if (!bLoaded)
			{
				UBlueprint* ObjectAsBlueprint = Cast<UBlueprint>(RequestedObject);
				if (ObjectAsBlueprint)
				{
					ObjectAsClass = ObjectAsBlueprint->GeneratedClass;
					if (ObjectAsClass && ObjectAsClass->IsChildOf<ULuauState>())
					{
						bLoaded = true;
					}
				}
			}

			if (bLoaded)
			{
				ULuauState* SpawnedState = FLuauMachineModule::Get().GetLuauState(ObjectAsClass, InWorld, false);
				if (SpawnedState)
				{
					UE_LOG(LogLuauMachine, Log, TEXT("%s spawned."), *SpawnedState->GetName());
				}
				return true;
			}

			UE_LOG(LogLuauMachine, Error, TEXT("specified argument is not a valid LuauState path."));
		}
	}

	return false;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FLuauMachineModule, LuauMachine)