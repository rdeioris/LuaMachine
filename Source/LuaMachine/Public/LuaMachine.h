// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

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

	ULuaState* GetLuaState(TSubclassOf<ULuaState> LuaStateClass, UWorld* InWorld, bool bCheckOnly=false);

private:
	TMap<TSubclassOf<ULuaState>, ULuaState*> LuaStates;
};
