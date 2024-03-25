// Copyright 2018-2023 - Roberto De Ioris

#include "LuaMachineEditor.h"
#include "Editor/UnrealEd/Public/Editor.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "EditorStyleSet.h"
#include "Runtime/Projects/Public/Interfaces/IPluginManager.h"
#include "LuaCodeCustomization.h"
#include "LuaValueCustomization.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructure.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"
#include "Runtime/Slate/Public/Widgets/Text/STextBlock.h"
#include "Runtime/Slate/Public/Widgets/Views/STreeView.h"
#include "Runtime/Slate/Public/Widgets/Input/SButton.h"
#include "Runtime/Slate/Public/Widgets/Input/STextComboBox.h"
#include "LuaMachine/Public/LuaMachine.h"
#include "LuaMachine/Public/LuaBlueprintFunctionLibrary.h"
#include "Widgets/Layout/SScrollBox.h"
#include "LuaUserDataObject.h"
#include "LuaCodeFactory.h"

#define LOCTEXT_NAMESPACE "FLuaMachineEditorModule"

FLuaMachineEditorModule::FLuaMachineEditorModule()
	: LuaMachineAssetCategoryBit(EAssetTypeCategories::Misc)
{

}

void FLuaMachineEditorModule::StartupModule()
{
	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FLuaMachineEditorModule::OnPostEngineInit);

	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	StyleSet = MakeShareable(new FSlateStyleSet("LuaMachineEditor"));
	StyleSet->SetContentRoot(IPluginManager::Get().FindPlugin("LuaMachine")->GetBaseDir() / "Resources");
	StyleSet->Set("ClassThumbnail.LuaCode", new FSlateImageBrush(StyleSet->RootToContentDir("Icon128.png"), FVector2D(128.0f, 128.0f)));
	StyleSet->Set("ClassThumbnail.LuaTableAsset", new FSlateImageBrush(StyleSet->RootToContentDir("LuaTableAsset256.png"), FVector2D(256.0f, 256.0f)));

	const FSlateFontInfo Consolas9 = FCoreStyle::GetDefaultFontStyle("Mono", 9);

	const FTextBlockStyle CodeBaseStyle = FTextBlockStyle()
		.SetFont(Consolas9)
		.SetColorAndOpacity(FLinearColor::White)
		.SetShadowOffset(FVector2D::ZeroVector)
		.SetShadowColorAndOpacity(FLinearColor::Black);

	StyleSet->Set("SyntaxHighlight.LuaMachine.Normal", CodeBaseStyle);
	StyleSet->Set("SyntaxHighlight.LuaMachine.Comment", FTextBlockStyle(CodeBaseStyle).SetColorAndOpacity(FLinearColor::Green));
	StyleSet->Set("SyntaxHighlight.LuaMachine.String", FTextBlockStyle(CodeBaseStyle).SetColorAndOpacity(FLinearColor(FColor::Orange)));
	StyleSet->Set("SyntaxHighlight.LuaMachine.Keyword", FTextBlockStyle(CodeBaseStyle).SetColorAndOpacity(FLinearColor(FColor::Cyan)));
	StyleSet->Set("SyntaxHighlight.LuaMachine.Nil", FTextBlockStyle(CodeBaseStyle).SetColorAndOpacity(FLinearColor::Red));
	StyleSet->Set("SyntaxHighlight.LuaMachine.StdLib", FTextBlockStyle(CodeBaseStyle).SetColorAndOpacity(FLinearColor::Yellow));
	StyleSet->Set("SyntaxHighlight.LuaMachine.Basic", FTextBlockStyle(CodeBaseStyle).SetColorAndOpacity(FLinearColor::FromSRGBColor(FColor::Magenta)));

#if ENGINE_MINOR_VERSION >= 3
	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
#endif

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));

	PropertyModule.RegisterCustomPropertyTypeLayout(TEXT("LuaValue"), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FLuaValueCustomization::MakeInstance));
	PropertyModule.RegisterCustomClassLayout(TEXT("LuaCode"), FOnGetDetailCustomizationInstance::CreateStatic(&FLuaCodeCustomization::MakeInstance));

	PropertyModule.NotifyCustomizationModuleChanged();

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner("LuaMachineDebugger", FOnSpawnTab::CreateStatic(&FLuaMachineEditorModule::CreateLuaMachineDebugger))
		.SetDisplayName(LOCTEXT("LuaMachine Debugger", "LuaMachine Debugger"))
		.SetTooltipText(LOCTEXT("Open the LuaMachine Debugger", "Open the LuaMachine Debugger"))
		.SetIcon(FSlateIcon(
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1
			FAppStyle::GetAppStyleSetName()
#else
			FEditorStyle::GetStyleSetName()
#endif
			, "DebugTools.TabIcon"))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsMiscCategory());
}

void FLuaMachineEditorModule::OnPostEngineInit()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	LuaMachineAssetCategoryBit = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("LuaMachine")), LOCTEXT("AssetCategory", "Lua Machine"));

	//Add LuaCode to Filters.
	RegisterAssetTypeAction(AssetTools, MakeShareable(new FLuaCodeAssetTypeActions(LuaMachineAssetCategoryBit)));
}

TSharedPtr<FSlateStyleSet> FLuaMachineEditorModule::GetStyleSet()
{
	return StyleSet;
}

void FLuaMachineEditorModule::RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action)
{
	AssetTools.RegisterAssetTypeActions(Action);
	CreatedAssetTypeActions.Add(Action);
}

struct FTableViewLuaValue : public TSharedFromThis<FTableViewLuaValue>
{
	FString LuaTableKey;
	FLuaValue LuaTableValue;
	bool bExpanded;
	TArray<TSharedRef<FTableViewLuaValue>> Children;
};

class SLuaMachineDebugger : public SCompoundWidget, public FGCObject
{
	SLATE_BEGIN_ARGS(SLuaMachineDebugger)
		{}

	SLATE_END_ARGS()

	void RebuildLuaValues()
	{
		LuaValues.Empty();

		if (!SelectedLuaState)
		{
			return;
		}

		SelectedLuaState->PushGlobalTable();
		SelectedLuaState->PushNil(); // first key
		while (SelectedLuaState->Next(-2))
		{
			FLuaValue CurrentLuaKey = SelectedLuaState->ToLuaValue(-2);
			FLuaValue CurrentLuaValue = SelectedLuaState->ToLuaValue(-1);
			TSharedRef<FTableViewLuaValue> LuaItem = MakeShared<FTableViewLuaValue>();
			LuaItem->LuaTableKey = CurrentLuaKey.ToString();
			LuaItem->LuaTableValue = CurrentLuaValue;
			LuaItem->bExpanded = false;
			LuaValues.Add(LuaItem); // add key
			SelectedLuaState->Pop(); // pop the value
		}

		SelectedLuaState->Pop(); // pop the table

		LuaValues.Sort([](const TSharedRef<FTableViewLuaValue>& LHS, const TSharedRef<FTableViewLuaValue>& RHS) { return LHS->LuaTableKey < RHS->LuaTableKey; });
	}

	void OnGetChildren(TSharedRef<FTableViewLuaValue> Item, TArray<TSharedRef<FTableViewLuaValue>>& OutChildren)
	{
		if (!SelectedLuaState)
			return;

		if (Item->LuaTableValue.Type == ELuaValueType::Table)
		{
			if (!Item->bExpanded)
			{
				SelectedLuaState->FromLuaValue(Item->LuaTableValue);
				SelectedLuaState->PushNil(); // first key
				while (SelectedLuaState->Next(-2))
				{
					FLuaValue CurrentLuaKey = SelectedLuaState->ToLuaValue(-2);
					FLuaValue CurrentLuaValue = SelectedLuaState->ToLuaValue(-1);
					TSharedRef<FTableViewLuaValue> LuaItem = MakeShared<FTableViewLuaValue>();
					LuaItem->LuaTableKey = CurrentLuaKey.ToString();
					LuaItem->LuaTableValue = CurrentLuaValue;
					LuaItem->bExpanded = false;
					Item->Children.Add(LuaItem); // add key
					SelectedLuaState->Pop(); // pop the value
				}
				SelectedLuaState->Pop(); // pop the table
				Item->bExpanded = true;
				Item->Children.Sort([](const TSharedRef<FTableViewLuaValue>& LHS, const TSharedRef<FTableViewLuaValue>& RHS) { return LHS->LuaTableKey < RHS->LuaTableKey; });
			}
			OutChildren.Append(Item->Children);
		}
		else if (Item->LuaTableValue.Type == ELuaValueType::UObject && Item->LuaTableValue.Object)
		{
			if (Item->LuaTableValue.Object->IsA<ULuaUserDataObject>())
			{
				if (!Item->bExpanded)
				{
					ULuaUserDataObject* LuaUserDataObject = Cast<ULuaUserDataObject>(Item->LuaTableValue.Object);
					if (LuaUserDataObject)
					{
						for (TPair<FString, FLuaValue> Pair : LuaUserDataObject->Table)
						{
							TSharedRef<FTableViewLuaValue> LuaItem = MakeShared<FTableViewLuaValue>();
							LuaItem->LuaTableKey = Pair.Key;
							LuaItem->LuaTableValue = Pair.Value;
							LuaItem->bExpanded = false;
							Item->Children.Add(LuaItem);
						}
					}
					Item->bExpanded = true;
					Item->Children.Sort([](const TSharedRef<FTableViewLuaValue>& LHS, const TSharedRef<FTableViewLuaValue>& RHS) { return LHS->LuaTableKey < RHS->LuaTableKey; });
				}
				OutChildren.Append(Item->Children);
			}
		}
	}

	void OnRegisteredLuaStatesChanged()
	{
		DetectedLuaStates.Empty();
		TArray<ULuaState*> States = FLuaMachineModule::Get().GetRegisteredLuaStates();
		for (ULuaState* State : States)
		{
			DetectedLuaStates.Add(MakeShared<FString>(State->GetClass()->GetName()));
		}

		if (LuaStatesComboBox.IsValid())
		{
			LuaStatesComboBox->RefreshOptions();
		}

		RefreshDebugText();
	}

	FReply CallGC()
	{
		GEngine->ForceGarbageCollection(true);
		return RefreshDebugger();
	}

	FReply CallLuaGC()
	{
		// avoid calling Lua GC on invalid lua state
		RefreshDebugger();
		if (SelectedLuaState && SelectedLuaState->GetInternalLuaState())
		{
			SelectedLuaState->GC(LUA_GCCOLLECT);
		}
		return RefreshDebugger();
	}

	void RefreshDebugText()
	{
		DebugTextContext.Empty();
		ReferencersTextContext.Empty();

		for (TObjectIterator<ULuaState> StatesIterator; StatesIterator; ++StatesIterator)
		{
			ULuaState* LuaState = *StatesIterator;
#if ENGINE_MAJOR_VERSION > 4
			if (LuaState->IsValidLowLevel() && !IsValid(LuaState))
#else
			if (LuaState->IsValidLowLevel() && !LuaState->IsPendingKill())
#endif
			{
				TArray<UObject*> Referencers;
				FReferenceFinder Collector(Referencers, nullptr, false, true, false, false);
				Collector.FindReferences(LuaState);

				if (LuaState->GetInternalLuaState())
				{
					LuaState->PushRegistryTable();
					int32 RegistrySize = LuaState->ILen(-1);
					LuaState->Pop();
					DebugTextContext += FString::Printf(TEXT("%s at 0x%p (%sused memory: %dk) (top of the stack: %d) (registry size: %d) (uobject refs: %d) (tracked user data: %d)\n"), *LuaState->GetName(), LuaState, LuaState->bPersistent ? TEXT("persistent, ") : TEXT(""), LuaState->GC(LUA_GCCOUNT), LuaState->GetTop(), RegistrySize, Referencers.Num(), LuaState->TrackedLuaUserDataObjects.Num());
				}
				else
				{
					DebugTextContext += FString::Printf(TEXT("%s at 0x%p (%sinactive) (uobject refs: %d)\n"), *LuaState->GetName(), LuaState, LuaState->bPersistent ? TEXT("persistent, ") : TEXT(""), Referencers.Num());
				}
			}
		}

		if (SelectedLuaState)
		{
			TArray<UObject*> Referencers;
			FReferenceFinder Collector(Referencers, nullptr, false, true, false, false);
			Collector.FindReferences(SelectedLuaState);

			for (UObject* Referencer : Referencers)
			{
				ReferencersTextContext += FString::Printf(TEXT("%s\n"), *Referencer->GetFullName());
			}
		}

		if (DebugText.IsValid())
		{
			DebugText->SetText(FText::FromString(DebugTextContext));
		}

		if (ReferencersText.IsValid())
		{
			ReferencersText->SetText(FText::FromString(ReferencersTextContext));
		}
	}

	FSlateColor GetLuaTypeColor(TSharedRef<FTableViewLuaValue> Item)
	{
		switch (Item->LuaTableValue.Type)
		{
		case ELuaValueType::Nil:
		case ELuaValueType::Number:
		case ELuaValueType::Integer:
			return FSlateColor(FColor::Red);
		case ELuaValueType::String:
			return FSlateColor(FColor::Orange);
		case ELuaValueType::Bool:
			return FSlateColor(FColor::Purple);
		case ELuaValueType::UFunction:
			return FSlateColor(FColor::Magenta);
		case ELuaValueType::UObject:
			return FSlateColor(FColor::Cyan);
		case ELuaValueType::Thread:
			return FSlateColor(FColor::Yellow);
		default:
			return FSlateColor(FLinearColor::White);
		}
	}

	FText GetLuaTypeText(TSharedRef<FTableViewLuaValue> Item)
	{
		FString Value = "nil";
		switch (Item->LuaTableValue.Type)
		{
		case ELuaValueType::String:
			Value = "string";
			break;
		case ELuaValueType::Bool:
			Value = "boolean";
			break;
		case ELuaValueType::Integer:
			Value = "integer";
			break;
		case ELuaValueType::Number:
			Value = "number";
			break;
		case ELuaValueType::Function:
			Value = "function";
			break;
		case ELuaValueType::Table:
			Value = "table";
			break;
		case ELuaValueType::Thread:
			Value = "thread";
			break;
		case ELuaValueType::UFunction:
			Value = "UFunction";
			break;
		case ELuaValueType::UObject:
			Value = "UObject";
			break;
		default:
			break;
		}

		return FText::FromString(Value);
	}

	FText GetLuaValueText(TSharedRef<FTableViewLuaValue> Item)
	{
		FString Value = "";
		switch (Item->LuaTableValue.Type)
		{
		case ELuaValueType::String:
		case ELuaValueType::Bool:
		case ELuaValueType::Integer:
		case ELuaValueType::Number:
		case ELuaValueType::UFunction:
		case ELuaValueType::UObject:
			Value = Item->LuaTableValue.ToString();
			break;
		case ELuaValueType::Thread:
			if (SelectedLuaState == Item->LuaTableValue.LuaState)
			{
				Value = "status: " + FindObject<UEnum>(nullptr, TEXT("/Script/LuaMachine.ELuaThreadStatus"), true)->GetNameStringByIndex((int32)SelectedLuaState->GetLuaThreadStatus(Item->LuaTableValue)) + ", stack top: " + FString::FromInt(SelectedLuaState->GetLuaThreadStackTop(Item->LuaTableValue));
			}
			break;
		default:
			break;
		}

		return FText::FromString(Value);
	}

	FReply RefreshDebugger()
	{
		if (LuaTreeView.IsValid())
		{
			if (LuaStatesComboBox.IsValid())
			{
				SelectedLuaState = nullptr;
				TSharedPtr<FString> SelectedText = LuaStatesComboBox->GetSelectedItem();
				if (SelectedText.IsValid())
				{
					TArray<ULuaState*> States = FLuaMachineModule::Get().GetRegisteredLuaStates();
					for (ULuaState* State : States)
					{
						if (State->GetClass()->GetName() == *SelectedText.Get())
						{
							if (State->GetInternalLuaState())
								SelectedLuaState = State;
							break;
						}
					}
				}
			}
			RebuildLuaValues();
			LuaTreeView->RequestTreeRefresh();
		}

		RefreshDebugText();
		return FReply::Handled();
	}

	TSharedRef<ITableRow> OnGenerateDebuggerRow(TSharedRef<FTableViewLuaValue> Item, const TSharedRef<STableViewBase>& OwnerTable)
	{
		return SNew(STableRow<TSharedRef<FTableViewLuaValue>>, OwnerTable)
			[
				SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					[
						SNew(STextBlock).Text(FText::FromString(Item->LuaTableKey))
					]
					+ SHorizontalBox::Slot().FillWidth(0.2)
					[
						SNew(STextBlock).Text(GetLuaTypeText(Item))
					]
					+ SHorizontalBox::Slot()
					[
						SNew(STextBlock).Text(GetLuaValueText(Item)).ColorAndOpacity(GetLuaTypeColor(Item)).ToolTipText(GetLuaValueText(Item))
					]
			];
	}

	void Construct(const FArguments& InArgs)
	{
		SelectedLuaState = nullptr;
		OnRegisteredLuaStatesChanged();
		RebuildLuaValues();
		ChildSlot[
			SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(EVerticalAlignment::VAlign_Center).HAlign(EHorizontalAlignment::HAlign_Left)
						[
							SNew(STextBlock).Text(FText::FromString("Select LuaState to Debug: "))
						]
						+ SHorizontalBox::Slot().FillWidth(0.6)
						[
							SAssignNew(LuaStatesComboBox, STextComboBox).OptionsSource(&DetectedLuaStates)
						]
				]

				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SButton).Text(FText::FromString("Refresh")).OnClicked(this, &SLuaMachineDebugger::RefreshDebugger)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SButton).Text(FText::FromString("Call Unreal GC")).OnClicked(this, &SLuaMachineDebugger::CallGC)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SButton).Text(FText::FromString("Call Lua GC")).OnClicked(this, &SLuaMachineDebugger::CallLuaGC)
				]
				+ SVerticalBox::Slot().FillHeight(1)
				[
					SNew(SScrollBox).AllowOverscroll(EAllowOverscroll::Yes)
						+ SScrollBox::Slot()
						[
							SAssignNew(LuaTreeView, STreeView<TSharedRef<FTableViewLuaValue>>).TreeItemsSource(&LuaValues).OnGetChildren(this, &SLuaMachineDebugger::OnGetChildren).OnGenerateRow(this, &SLuaMachineDebugger::OnGenerateDebuggerRow)
						]
				]
				+ SVerticalBox::Slot().FillHeight(0.1)
				[
					SNew(SBorder).BorderBackgroundColor(FColor::White).Padding(4)
						[
							SNew(SScrollBox).AllowOverscroll(EAllowOverscroll::Yes)
								+ SScrollBox::Slot()
								[
									SAssignNew(ReferencersText, STextBlock).Text(FText::FromString(ReferencersTextContext))
								]
						]
				]
				+ SVerticalBox::Slot().VAlign(EVerticalAlignment::VAlign_Bottom).AutoHeight()
				[
					SNew(SBorder).BorderBackgroundColor(FColor::Red).Padding(4)
						[
							SAssignNew(DebugText, STextBlock).Text(FText::FromString(DebugTextContext))
						]
				]
		];
		FLuaMachineModule::Get().OnRegisteredLuaStatesChanged.AddSP(this, &SLuaMachineDebugger::OnRegisteredLuaStatesChanged);
	}

	void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		Collector.AddReferencedObject(SelectedLuaState);
	}

#if ENGINE_MAJOR_VERSION > 4
	virtual FString GetReferencerName() const override
	{
		return TEXT("SLuaMachineDebugger");
	}
#endif

protected:
	TArray<TSharedRef<FTableViewLuaValue>> LuaValues;
	TSharedPtr<STreeView<TSharedRef<FTableViewLuaValue>>> LuaTreeView;
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 4
	TObjectPtr<ULuaState> SelectedLuaState;
#else
	ULuaState* SelectedLuaState;
#endif
	TArray<TSharedPtr<FString>> DetectedLuaStates;
	TSharedPtr<STextComboBox> LuaStatesComboBox;
	TSharedPtr<STextBlock> DebugText;
	FString DebugTextContext;
	TSharedPtr<STextBlock> ReferencersText;
	FString ReferencersTextContext;
};

TSharedRef<SDockTab> FLuaMachineEditorModule::CreateLuaMachineDebugger(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab).TabRole(ETabRole::NomadTab)
		[
			SNew(SLuaMachineDebugger)
		];
}

void FLuaMachineEditorModule::ShutdownModule()
{
	FCoreDelegates::OnPostEngineInit.RemoveAll(this);

	// Unregister all the asset types that we registered
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		for (int32 Index = 0; Index < CreatedAssetTypeActions.Num(); ++Index)
		{
			AssetTools.UnregisterAssetTypeActions(CreatedAssetTypeActions[Index].ToSharedRef());
		}
	}
	CreatedAssetTypeActions.Empty();
}

FLuaMachineEditorModule& FLuaMachineEditorModule::Get()
{
	static FLuaMachineEditorModule* Singleton = nullptr;
	if (Singleton == nullptr)
	{
		Singleton = &FModuleManager::LoadModuleChecked<FLuaMachineEditorModule>("LuaMachineEditor");
	}
	return *Singleton;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FLuaMachineEditorModule, LuaMachineEditor)