// Copyright 2018-2020 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "LuaState.h"
#include "SlateCore/Public/Styling/SlateStyle.h"
#include "Runtime/Slate/Public/Widgets/Docking/SDockTab.h"

class FLuaMachineEditorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static FLuaMachineEditorModule& Get();

	TSharedPtr<FSlateStyleSet> GetStyleSet();

private:
	TSharedPtr<FSlateStyleSet> StyleSet;
	static TSharedRef<SDockTab> CreateLuaMachineDebugger(const FSpawnTabArgs& Args);
};
