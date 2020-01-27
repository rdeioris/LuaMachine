// Copyright 2018-2020 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Runtime/Launch/Resources/Version.h"
#include "LuaState.h"

DECLARE_MULTICAST_DELEGATE(FOnRegisteredLuaStatesChanged);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnNewLuaState, ULuaState*);

class LUAMACHINE_API FLuaMachineModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static FLuaMachineModule& Get();

	void CleanupLuaStates(bool bIsSimulating);
	void UnregisterLuaState(ULuaState* LuaState);

	ULuaState* GetLuaState(TSubclassOf<ULuaState> LuaStateClass, UWorld* InWorld, bool bCheckOnly=false);

	TArray<ULuaState*> GetRegisteredLuaStates();

	FOnNewLuaState OnNewLuaState;
	FOnRegisteredLuaStatesChanged OnRegisteredLuaStatesChanged;

private:
	TMap<TSubclassOf<ULuaState>, ULuaState*> LuaStates;
};
