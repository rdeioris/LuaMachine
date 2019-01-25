// Copyright 2019 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "LuaState.h"


class FLuaMachineModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static inline FLuaMachineModule& Get();

	void CleanupLuaStates(bool bIsSimulating);
	void UnregisterLuaState(ULuaState* LuaState);

	ULuaState* GetLuaState(TSubclassOf<ULuaState> LuaStateClass, UWorld* InWorld, bool bCheckOnly=false);

	TArray<ULuaState*> GetRegisteredLuaStates();

private:
	TMap<TSubclassOf<ULuaState>, ULuaState*> LuaStates;
};
