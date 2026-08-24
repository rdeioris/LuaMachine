// Copyright 2018-2025 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Modules/ModuleManager.h"

class FLuaMachineBlueprintNodesModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
};
