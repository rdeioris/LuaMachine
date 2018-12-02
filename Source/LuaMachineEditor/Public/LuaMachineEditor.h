// Copyright 2018 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "LuaState.h"
#include "SlateCore/Public/Styling/SlateStyle.h"

class FLuaMachineEditorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static inline FLuaMachineEditorModule& Get();

	TSharedPtr<FSlateStyleSet> GetStyleSet();

private:
	TSharedPtr<FSlateStyleSet> StyleSet;
};
