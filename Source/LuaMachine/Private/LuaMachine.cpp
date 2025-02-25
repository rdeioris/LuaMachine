// Copyright 2018-2023 - Roberto De Ioris

#include "LuaMachine.h"
#include "LuaBlueprintFunctionLibrary.h"
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

#define LOCTEXT_NAMESPACE "FLuaMachineModule"

void FLuaMachineModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
#if WITH_EDITOR
	FEditorDelegates::BeginPIE.AddRaw(this, &FLuaMachineModule::CleanupLuaStates);
	FEditorDelegates::EndPIE.AddRaw(this, &FLuaMachineModule::CleanupLuaStates);
#endif

	// streaming level hooks
	FWorldDelegates::LevelAddedToWorld.AddRaw(this, &FLuaMachineModule::LuaLevelAddedToWorld);
	FWorldDelegates::LevelRemovedFromWorld.AddRaw(this, &FLuaMachineModule::LuaLevelRemovedFromWorld);

}

void FLuaMachineModule::LuaLevelAddedToWorld(ULevel* Level, UWorld* World)
{
	for (ULuaState* LuaState : GetRegisteredLuaStates())
	{
#if ENGINE_MAJOR_VERSION > 4
		if (LuaState->IsValidLowLevel() && !IsValid(LuaState))
#else
		if (LuaState->IsValidLowLevel() && !LuaState->IsPendingKill())
#endif
		{
			LuaState->ReceiveLuaLevelAddedToWorld(Level, World);
		}
	}
}

void FLuaMachineModule::LuaLevelRemovedFromWorld(ULevel* Level, UWorld* World)
{
	for (ULuaState* LuaState : GetRegisteredLuaStates())
	{
#if ENGINE_MAJOR_VERSION > 4
		if (LuaState->IsValidLowLevel() && !IsValid(LuaState))
#else
		if (LuaState->IsValidLowLevel() && !LuaState->IsPendingKill())
#endif
		{
			LuaState->ReceiveLuaLevelRemovedFromWorld(Level, World);
		}
	}
}

void FLuaMachineModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

void FLuaMachineModule::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObjects(LuaStates);
}

void FLuaMachineModule::CleanupLuaStates(bool bIsSimulating)
{
	TArray<TSubclassOf<ULuaState>> LuaStatesKeys;
	LuaStates.GetKeys(LuaStatesKeys);

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 4
	TMap<TSubclassOf<ULuaState>, TObjectPtr<ULuaState>> PersistentLuaStates;
#else
	TMap<TSubclassOf<ULuaState>, ULuaState*> PersistentLuaStates;
#endif

	for (TSubclassOf<ULuaState> LuaStateClass : LuaStatesKeys)
	{
		if (LuaStates[LuaStateClass]->bPersistent)
		{
			PersistentLuaStates.Add(LuaStateClass, LuaStates[LuaStateClass]);
		}
		else
		{
			if (FLuaCommandExecutor* LuaConsole = LuaStates[LuaStateClass]->GetLuaConsole())
			{
				IModularFeatures::Get().UnregisterModularFeature(IConsoleCommandExecutor::ModularFeatureName(), LuaConsole);
			}
		}
	}

	LuaStates = PersistentLuaStates;
	OnRegisteredLuaStatesChanged.Broadcast();
}

ULuaState* FLuaMachineModule::GetLuaState(TSubclassOf<ULuaState> LuaStateClass, UWorld* InWorld, bool bCheckOnly)
{
	if (!LuaStateClass)
	{
		return nullptr;
	}

	if (LuaStateClass == ULuaState::StaticClass())
	{
		UE_LOG(LogLuaMachine, Error, TEXT("attempt to use LuaState Abstract class, please create a child of LuaState"));
		return nullptr;
	}

	if (!LuaStates.Contains(LuaStateClass))
	{
		if (bCheckOnly)
		{
			return nullptr;
		}
		ULuaState* NewLuaState = NewObject<ULuaState>((UObject*)GetTransientPackage(), LuaStateClass);
		LuaStates.Add(LuaStateClass, NewLuaState);
		OnNewLuaState.Broadcast(NewLuaState);
		OnRegisteredLuaStatesChanged.Broadcast();
	}

	return LuaStates[LuaStateClass]->GetLuaState(InWorld);
}

TArray<ULuaState*> FLuaMachineModule::GetRegisteredLuaStates()
{
	TArray<ULuaState*> RegisteredStates;
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 4
	for (TPair< TSubclassOf<ULuaState>, TObjectPtr<ULuaState>>& Pair : LuaStates)
#else
	for (TPair< TSubclassOf<ULuaState>, ULuaState*>& Pair : LuaStates)
#endif
	{
		RegisteredStates.Add(Pair.Value);
	}

	return RegisteredStates;
}

void FLuaMachineModule::UnregisterLuaState(ULuaState* LuaState)
{
	TSubclassOf<ULuaState> FoundLuaStateClass = nullptr;
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 4
	for (TPair< TSubclassOf<ULuaState>, TObjectPtr<ULuaState>>& Pair : LuaStates)
#else
	for (TPair< TSubclassOf<ULuaState>, ULuaState*>& Pair : LuaStates)
#endif
	{
		if (Pair.Value == LuaState)
		{
			FoundLuaStateClass = Pair.Key;
			break;
		}
	}

	if (FoundLuaStateClass)
	{
		LuaStates.Remove(FoundLuaStateClass);
	}

	// trick for waking up on low-level destructor
	OnRegisteredLuaStatesChanged.Broadcast();
}

void FLuaMachineModule::RegisterLuaConsoleCommand(const FString& CommandName, const FLuaValue& LuaConsoleCommand)
{
	// first check in lua console commands
	UnregisterLuaConsoleCommand(CommandName);
	if (IConsoleManager::Get().FindConsoleObject(*CommandName))
	{
		UE_LOG(LogLuaMachine, Error, TEXT("Lua Console Command Name \"%s\" is already in use."), *CommandName);
		return;
	}

	if (IConsoleManager::Get().RegisterConsoleCommand(*CommandName, *CommandName, FConsoleCommandWithArgsDelegate::CreateRaw(this, &FLuaMachineModule::RunLuaConsoleCommand, LuaConsoleCommand)))
	{
		LuaConsoleCommands.Add(CommandName);
	}
}

void FLuaMachineModule::UnregisterLuaConsoleCommand(const FString& CommandName)
{
	if (LuaConsoleCommands.Contains(CommandName))
	{
		IConsoleManager::Get().UnregisterConsoleObject(*CommandName);
		LuaConsoleCommands.Remove(CommandName);
	}
}

void FLuaMachineModule::RunLuaConsoleCommand(const TArray<FString>& Args, FLuaValue LuaConsoleCommand)
{
	TArray<FLuaValue> LuaArgs;
	for (FString Arg : Args)
	{
		LuaArgs.Add(FLuaValue(Arg));
	}
	ULuaBlueprintFunctionLibrary::LuaValueCallIfNotNil(LuaConsoleCommand, LuaArgs);
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

bool FLuaMachineModule::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	if (FParse::Command(&Cmd, TEXT("luaspawn")))
	{
		if (!*Cmd)
		{
			UE_LOG(LogLuaMachine, Error, TEXT("please specify a valid LuaState path."));
			return false;
		}

		UObject* RequestedObject = LoadObject<UObject>(nullptr, Cmd);
		if (RequestedObject)
		{
			bool bLoaded = false;
			UClass* ObjectAsClass = Cast<UClass>(RequestedObject);
			if (ObjectAsClass && ObjectAsClass->IsChildOf<ULuaState>())
			{
				bLoaded = true;
			}

			if (!bLoaded)
			{
				UBlueprint* ObjectAsBlueprint = Cast<UBlueprint>(RequestedObject);
				if (ObjectAsBlueprint)
				{
					ObjectAsClass = ObjectAsBlueprint->GeneratedClass;
					if (ObjectAsClass && ObjectAsClass->IsChildOf<ULuaState>())
					{
						bLoaded = true;
					}
				}
			}

			if (bLoaded)
			{
				ULuaState* SpawnedState = FLuaMachineModule::Get().GetLuaState(ObjectAsClass, InWorld, false);
				if (SpawnedState)
				{
					UE_LOG(LogLuaMachine, Log, TEXT("%s spawned."), *SpawnedState->GetName());
				}
				return true;
			}

			UE_LOG(LogLuaMachine, Error, TEXT("specified argument is not a valid LuaState path."));
		}
	}

	return false;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FLuaMachineModule, LuaMachine)