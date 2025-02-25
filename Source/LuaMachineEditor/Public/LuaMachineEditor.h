// Copyright 2018-2023 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Modules/ModuleManager.h"
#include "LuaState.h"
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 3
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#else
#include "SlateCore/Public/Styling/SlateStyle.h"
#endif
#include "Runtime/Slate/Public/Widgets/Docking/SDockTab.h"
#include "IAssetTools.h"
#include "IAssetTypeActions.h"
#include "AssetTypeCategories.h"

class FLuaMachineEditorModule : public IModuleInterface
{
public:
	FLuaMachineEditorModule();

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual void OnPostEngineInit();
	static FLuaMachineEditorModule& Get();

	TSharedPtr<FSlateStyleSet> GetStyleSet();

	virtual uint32 GetAssetCategory() const { return LuaMachineAssetCategoryBit; }
	void RegisterAssetTypeAction( IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action );
private:
	TSharedPtr<FSlateStyleSet> StyleSet;
	static TSharedRef<SDockTab> CreateLuaMachineDebugger(const FSpawnTabArgs& Args); 
	EAssetTypeCategories::Type LuaMachineAssetCategoryBit;
	/** All created asset type actions.  Cached here so that we can unregister them during shutdown. */
	TArray< TSharedPtr<IAssetTypeActions> > CreatedAssetTypeActions;
};
