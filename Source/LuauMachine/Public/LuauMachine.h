// Copyright 2018-2023 - Roberto De Ioris
// Portions Copyright 2025 - Fusion Point Studios, Inc.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "UObject/GCObject.h"
#include "LuauState.h"
#include "HAL/IConsoleManager.h"

DECLARE_MULTICAST_DELEGATE(FOnRegisteredLuauStatesChanged);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnNewLuauState, ULuauState*);

class LUAUMACHINE_API FLuauMachineModule : public IModuleInterface, public FGCObject, public FSelfRegisteringExec
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static FLuauMachineModule& Get();

	void CleanupLuauStates(bool bIsSimulating);
	void UnregisterLuauState(ULuauState* LuauState);

	ULuauState* GetLuauState(TSubclassOf<ULuauState> LuauStateClass, UWorld* InWorld, bool bCheckOnly=false);

	TArray<ULuauState*> GetRegisteredLuauStates();

	FOnNewLuauState OnNewLuauState;
	FOnRegisteredLuauStatesChanged OnRegisteredLuauStatesChanged;

	void LuauLevelAddedToWorld(ULevel* Level, UWorld* World);
	void LuauLevelRemovedFromWorld(ULevel* Level, UWorld* World);

	void AddReferencedObjects(FReferenceCollector& Collector) override;

	void RegisterLuauConsoleCommand(const FString& CommandName, const FLuauValue& LuauConsoleCommand);
	void UnregisterLuauConsoleCommand(const FString& CommandName);

	void RunLuauConsoleCommand(const TArray<FString>& Args, FLuauValue LuauConsoleCommand);

#if ENGINE_MAJOR_VERSION > 4
	virtual FString GetReferencerName() const override
	{
		return TEXT("FLuauMachineModule");
	}
#endif

	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar);

private:
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 4
	TMap<TSubclassOf<ULuauState>, TObjectPtr<ULuauState>> LuauStates;
#else
	TMap<TSubclassOf<ULuauState>, ULuauState*> LuauStates;
#endif
	TSet<FString> LuauConsoleCommands;
};
