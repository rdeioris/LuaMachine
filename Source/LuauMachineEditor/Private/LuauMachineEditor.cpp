// Copyright 2018-2023 - Roberto De Ioris

#include "LuauMachineEditor.h"
#include "Editor/UnrealEd/Public/Editor.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "EditorStyleSet.h"
#include "Runtime/Projects/Public/Interfaces/IPluginManager.h"
#include "LuauCodeCustomization.h"
#include "LuauValueCustomization.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructure.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"
#include "Runtime/Slate/Public/Widgets/Text/STextBlock.h"
#include "Runtime/Slate/Public/Widgets/Views/STreeView.h"
#include "Runtime/Slate/Public/Widgets/Input/SButton.h"
#include "Runtime/Slate/Public/Widgets/Input/STextComboBox.h"
#include "LuauMachine/Public/LuauMachine.h"
#include "LuauMachine/Public/LuauBlueprintFunctionLibrary.h"
#include "Widgets/Layout/SScrollBox.h"
#include "LuauUserDataObject.h"
#include "LuauCodeFactory.h"

#define LOCTEXT_NAMESPACE "FLuauMachineEditorModule"

FLuauMachineEditorModule::FLuauMachineEditorModule()
	: LuauMachineAssetCategoryBit(EAssetTypeCategories::Misc)
{

}

void FLuauMachineEditorModule::StartupModule()
{
	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FLuauMachineEditorModule::OnPostEngineInit);

	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	StyleSet = MakeShareable(new FSlateStyleSet("LuauMachineEditor"));
	StyleSet->SetContentRoot(IPluginManager::Get().FindPlugin("LuauMachine")->GetBaseDir() / "Resources");
	StyleSet->Set("ClassThumbnail.LuauCode", new FSlateImageBrush(StyleSet->RootToContentDir("Icon128.png"), FVector2D(128.0f, 128.0f)));
	StyleSet->Set("ClassThumbnail.LuauTableAsset", new FSlateImageBrush(StyleSet->RootToContentDir("LuauTableAsset256.png"), FVector2D(256.0f, 256.0f)));

	const FSlateFontInfo Consolas9 = FCoreStyle::GetDefaultFontStyle("Mono", 9);

	const FTextBlockStyle CodeBaseStyle = FTextBlockStyle()
		.SetFont(Consolas9)
		.SetColorAndOpacity(FLinearColor::White)
		.SetShadowOffset(FVector2D::ZeroVector)
		.SetShadowColorAndOpacity(FLinearColor::Black);

	StyleSet->Set("SyntaxHighlight.LuauMachine.Normal", CodeBaseStyle);
	StyleSet->Set("SyntaxHighlight.LuauMachine.Comment", FTextBlockStyle(CodeBaseStyle).SetColorAndOpacity(FLinearColor::Green));
	StyleSet->Set("SyntaxHighlight.LuauMachine.String", FTextBlockStyle(CodeBaseStyle).SetColorAndOpacity(FLinearColor(FColor::Orange)));
	StyleSet->Set("SyntaxHighlight.LuauMachine.Keyword", FTextBlockStyle(CodeBaseStyle).SetColorAndOpacity(FLinearColor(FColor::Cyan)));
	StyleSet->Set("SyntaxHighlight.LuauMachine.Nil", FTextBlockStyle(CodeBaseStyle).SetColorAndOpacity(FLinearColor::Red));
	StyleSet->Set("SyntaxHighlight.LuauMachine.StdLib", FTextBlockStyle(CodeBaseStyle).SetColorAndOpacity(FLinearColor::Yellow));
	StyleSet->Set("SyntaxHighlight.LuauMachine.Basic", FTextBlockStyle(CodeBaseStyle).SetColorAndOpacity(FLinearColor::FromSRGBColor(FColor::Magenta)));

#if ENGINE_MINOR_VERSION >= 3
	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
#endif

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));

	PropertyModule.RegisterCustomPropertyTypeLayout(TEXT("LuauValue"), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FLuauValueCustomization::MakeInstance));
	PropertyModule.RegisterCustomClassLayout(TEXT("LuauCode"), FOnGetDetailCustomizationInstance::CreateStatic(&FLuauCodeCustomization::MakeInstance));

	PropertyModule.NotifyCustomizationModuleChanged();

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner("LuauMachineDebugger", FOnSpawnTab::CreateStatic(&FLuauMachineEditorModule::CreateLuauMachineDebugger))
		.SetDisplayName(LOCTEXT("LuauMachine Debugger", "LuauMachine Debugger"))
		.SetTooltipText(LOCTEXT("Open the LuauMachine Debugger", "Open the LuauMachine Debugger"))
		.SetIcon(FSlateIcon(
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1
			FAppStyle::GetAppStyleSetName()
#else
			FEditorStyle::GetStyleSetName()
#endif
			, "DebugTools.TabIcon"))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsMiscCategory());
}

void FLuauMachineEditorModule::OnPostEngineInit()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	LuauMachineAssetCategoryBit = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("LuauMachine")), LOCTEXT("AssetCategory", "Luau Machine"));

	//Add LuauCode to Filters.
	RegisterAssetTypeAction(AssetTools, MakeShareable(new FLuauCodeAssetTypeActions(LuauMachineAssetCategoryBit)));
}

TSharedPtr<FSlateStyleSet> FLuauMachineEditorModule::GetStyleSet()
{
	return StyleSet;
}

void FLuauMachineEditorModule::RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action)
{
	AssetTools.RegisterAssetTypeActions(Action);
	CreatedAssetTypeActions.Add(Action);
}

struct FTableViewLuauValue : public TSharedFromThis<FTableViewLuauValue>
{
	FString LuauTableKey;
	FLuauValue LuauTableValue;
	bool bExpanded;
	TArray<TSharedRef<FTableViewLuauValue>> Children;
};

class SLuauMachineDebugger : public SCompoundWidget, public FGCObject
{
	SLATE_BEGIN_ARGS(SLuauMachineDebugger)
		{}

	SLATE_END_ARGS()

	void RebuildLuauValues()
	{
		LuauValues.Empty();

		if (!SelectedLuauState)
		{
			return;
		}

		SelectedLuauState->PushGlobalTable();
		SelectedLuauState->PushNil(); // first key
		while (SelectedLuauState->Next(-2))
		{
			FLuauValue CurrentLuauKey = SelectedLuauState->ToLuauValue(-2);
			FLuauValue CurrentLuauValue = SelectedLuauState->ToLuauValue(-1);
			TSharedRef<FTableViewLuauValue> LuauItem = MakeShared<FTableViewLuauValue>();
			LuauItem->LuauTableKey = CurrentLuauKey.ToString();
			LuauItem->LuauTableValue = CurrentLuauValue;
			LuauItem->bExpanded = false;
			LuauValues.Add(LuauItem); // add key
			SelectedLuauState->Pop(); // pop the value
		}

		SelectedLuauState->Pop(); // pop the table

		LuauValues.Sort([](const TSharedRef<FTableViewLuauValue>& LHS, const TSharedRef<FTableViewLuauValue>& RHS) { return LHS->LuauTableKey < RHS->LuauTableKey; });
	}

	void OnGetChildren(TSharedRef<FTableViewLuauValue> Item, TArray<TSharedRef<FTableViewLuauValue>>& OutChildren)
	{
		if (!SelectedLuauState)
			return;

		if (Item->LuauTableValue.Type == ELuauValueType::Table)
		{
			if (!Item->bExpanded)
			{
				SelectedLuauState->FromLuauValue(Item->LuauTableValue);
				SelectedLuauState->PushNil(); // first key
				while (SelectedLuauState->Next(-2))
				{
					FLuauValue CurrentLuauKey = SelectedLuauState->ToLuauValue(-2);
					FLuauValue CurrentLuauValue = SelectedLuauState->ToLuauValue(-1);
					TSharedRef<FTableViewLuauValue> LuauItem = MakeShared<FTableViewLuauValue>();
					LuauItem->LuauTableKey = CurrentLuauKey.ToString();
					LuauItem->LuauTableValue = CurrentLuauValue;
					LuauItem->bExpanded = false;
					Item->Children.Add(LuauItem); // add key
					SelectedLuauState->Pop(); // pop the value
				}
				SelectedLuauState->Pop(); // pop the table
				Item->bExpanded = true;
				Item->Children.Sort([](const TSharedRef<FTableViewLuauValue>& LHS, const TSharedRef<FTableViewLuauValue>& RHS) { return LHS->LuauTableKey < RHS->LuauTableKey; });
			}
			OutChildren.Append(Item->Children);
		}
		else if (Item->LuauTableValue.Type == ELuauValueType::UObject && Item->LuauTableValue.Object)
		{
			if (Item->LuauTableValue.Object->IsA<ULuauUserDataObject>())
			{
				if (!Item->bExpanded)
				{
					ULuauUserDataObject* LuauUserDataObject = Cast<ULuauUserDataObject>(Item->LuauTableValue.Object);
					if (LuauUserDataObject)
					{
						for (TPair<FString, FLuauValue> Pair : LuauUserDataObject->Table)
						{
							TSharedRef<FTableViewLuauValue> LuauItem = MakeShared<FTableViewLuauValue>();
							LuauItem->LuauTableKey = Pair.Key;
							LuauItem->LuauTableValue = Pair.Value;
							LuauItem->bExpanded = false;
							Item->Children.Add(LuauItem);
						}
					}
					Item->bExpanded = true;
					Item->Children.Sort([](const TSharedRef<FTableViewLuauValue>& LHS, const TSharedRef<FTableViewLuauValue>& RHS) { return LHS->LuauTableKey < RHS->LuauTableKey; });
				}
				OutChildren.Append(Item->Children);
			}
		}
	}

	void OnRegisteredLuauStatesChanged()
	{
		DetectedLuauStates.Empty();
		TArray<ULuauState*> States = FLuauMachineModule::Get().GetRegisteredLuauStates();
		for (ULuauState* State : States)
		{
			DetectedLuauStates.Add(MakeShared<FString>(State->GetClass()->GetName()));
		}

		if (LuauStatesComboBox.IsValid())
		{
			LuauStatesComboBox->RefreshOptions();
		}

		RefreshDebugText();
	}

	FReply CallGC()
	{
		GEngine->ForceGarbageCollection(true);
		return RefreshDebugger();
	}

	FReply CallLuauGC()
	{
		// avoid calling Luau GC on invalid lua state
		RefreshDebugger();
		if (SelectedLuauState && SelectedLuauState->GetInternalLuauState())
		{
			SelectedLuauState->GC(LUA_GCCOLLECT);
		}
		return RefreshDebugger();
	}

	void RefreshDebugText()
	{
		DebugTextContext.Empty();
		ReferencersTextContext.Empty();

		for (TObjectIterator<ULuauState> StatesIterator; StatesIterator; ++StatesIterator)
		{
			ULuauState* LuauState = *StatesIterator;
#if ENGINE_MAJOR_VERSION > 4
			if (LuauState->IsValidLowLevel() && !IsValid(LuauState))
#else
			if (LuauState->IsValidLowLevel() && !LuauState->IsPendingKill())
#endif
			{
				TArray<UObject*> Referencers;
				FReferenceFinder Collector(Referencers, nullptr, false, true, false, false);
				Collector.FindReferences(LuauState);

				if (LuauState->GetInternalLuauState())
				{
					LuauState->PushRegistryTable();
					int32 RegistrySize = LuauState->ILen(-1);
					LuauState->Pop();
					DebugTextContext += FString::Printf(TEXT("%s at 0x%p (%sused memory: %dk) (top of the stack: %d) (registry size: %d) (uobject refs: %d) (tracked user data: %d)\n"), *LuauState->GetName(), LuauState, LuauState->bPersistent ? TEXT("persistent, ") : TEXT(""), LuauState->GC(LUA_GCCOUNT), LuauState->GetTop(), RegistrySize, Referencers.Num(), LuauState->TrackedLuauUserDataObjects.Num());
				}
				else
				{
					DebugTextContext += FString::Printf(TEXT("%s at 0x%p (%sinactive) (uobject refs: %d)\n"), *LuauState->GetName(), LuauState, LuauState->bPersistent ? TEXT("persistent, ") : TEXT(""), Referencers.Num());
				}
			}
		}

		if (SelectedLuauState)
		{
			TArray<UObject*> Referencers;
			FReferenceFinder Collector(Referencers, nullptr, false, true, false, false);
			Collector.FindReferences(SelectedLuauState);

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

	FSlateColor GetLuauTypeColor(TSharedRef<FTableViewLuauValue> Item)
	{
		switch (Item->LuauTableValue.Type)
		{
		case ELuauValueType::Nil:
		case ELuauValueType::Number:
		case ELuauValueType::Integer:
			return FSlateColor(FColor::Red);
		case ELuauValueType::String:
			return FSlateColor(FColor::Orange);
		case ELuauValueType::Bool:
			return FSlateColor(FColor::Purple);
		case ELuauValueType::UFunction:
			return FSlateColor(FColor::Magenta);
		case ELuauValueType::UObject:
			return FSlateColor(FColor::Cyan);
		case ELuauValueType::Thread:
			return FSlateColor(FColor::Yellow);
		default:
			return FSlateColor(FLinearColor::White);
		}
	}

	FText GetLuauTypeText(TSharedRef<FTableViewLuauValue> Item)
	{
		FString Value = "nil";
		switch (Item->LuauTableValue.Type)
		{
		case ELuauValueType::String:
			Value = "string";
			break;
		case ELuauValueType::Bool:
			Value = "boolean";
			break;
		case ELuauValueType::Integer:
			Value = "integer";
			break;
		case ELuauValueType::Number:
			Value = "number";
			break;
		case ELuauValueType::Function:
			Value = "function";
			break;
		case ELuauValueType::Table:
			Value = "table";
			break;
		case ELuauValueType::Thread:
			Value = "thread";
			break;
		case ELuauValueType::UFunction:
			Value = "UFunction";
			break;
		case ELuauValueType::UObject:
			Value = "UObject";
			break;
		default:
			break;
		}

		return FText::FromString(Value);
	}

	FText GetLuauValueText(TSharedRef<FTableViewLuauValue> Item)
	{
		FString Value = "";
		switch (Item->LuauTableValue.Type)
		{
		case ELuauValueType::String:
		case ELuauValueType::Bool:
		case ELuauValueType::Integer:
		case ELuauValueType::Number:
		case ELuauValueType::UFunction:
		case ELuauValueType::UObject:
			Value = Item->LuauTableValue.ToString();
			break;
		case ELuauValueType::Thread:
			if (SelectedLuauState == Item->LuauTableValue.LuauState)
			{
				Value = "status: " + FindObject<UEnum>(nullptr, TEXT("/Script/LuauMachine.ELuauThreadStatus"), true)->GetNameStringByIndex((int32)SelectedLuauState->GetLuauThreadStatus(Item->LuauTableValue)) + ", stack top: " + FString::FromInt(SelectedLuauState->GetLuauThreadStackTop(Item->LuauTableValue));
			}
			break;
		default:
			break;
		}

		return FText::FromString(Value);
	}

	FReply RefreshDebugger()
	{
		if (LuauTreeView.IsValid())
		{
			if (LuauStatesComboBox.IsValid())
			{
				SelectedLuauState = nullptr;
				TSharedPtr<FString> SelectedText = LuauStatesComboBox->GetSelectedItem();
				if (SelectedText.IsValid())
				{
					TArray<ULuauState*> States = FLuauMachineModule::Get().GetRegisteredLuauStates();
					for (ULuauState* State : States)
					{
						if (State->GetClass()->GetName() == *SelectedText.Get())
						{
							if (State->GetInternalLuauState())
								SelectedLuauState = State;
							break;
						}
					}
				}
			}
			RebuildLuauValues();
			LuauTreeView->RequestTreeRefresh();
		}

		RefreshDebugText();
		return FReply::Handled();
	}

	TSharedRef<ITableRow> OnGenerateDebuggerRow(TSharedRef<FTableViewLuauValue> Item, const TSharedRef<STableViewBase>& OwnerTable)
	{
		return SNew(STableRow<TSharedRef<FTableViewLuauValue>>, OwnerTable)
			[
				SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					[
						SNew(STextBlock).Text(FText::FromString(Item->LuauTableKey))
					]
					+ SHorizontalBox::Slot().FillWidth(0.2)
					[
						SNew(STextBlock).Text(GetLuauTypeText(Item))
					]
					+ SHorizontalBox::Slot()
					[
						SNew(STextBlock).Text(GetLuauValueText(Item)).ColorAndOpacity(GetLuauTypeColor(Item)).ToolTipText(GetLuauValueText(Item))
					]
			];
	}

	void Construct(const FArguments& InArgs)
	{
		SelectedLuauState = nullptr;
		OnRegisteredLuauStatesChanged();
		RebuildLuauValues();
		ChildSlot[
			SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(EVerticalAlignment::VAlign_Center).HAlign(EHorizontalAlignment::HAlign_Left)
						[
							SNew(STextBlock).Text(FText::FromString("Select LuauState to Debug: "))
						]
						+ SHorizontalBox::Slot().FillWidth(0.6)
						[
							SAssignNew(LuauStatesComboBox, STextComboBox).OptionsSource(&DetectedLuauStates)
						]
				]

				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SButton).Text(FText::FromString("Refresh")).OnClicked(this, &SLuauMachineDebugger::RefreshDebugger)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SButton).Text(FText::FromString("Call Unreal GC")).OnClicked(this, &SLuauMachineDebugger::CallGC)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SButton).Text(FText::FromString("Call Luau GC")).OnClicked(this, &SLuauMachineDebugger::CallLuauGC)
				]
				+ SVerticalBox::Slot().FillHeight(1)
				[
					SNew(SScrollBox).AllowOverscroll(EAllowOverscroll::Yes)
						+ SScrollBox::Slot()
						[
							SAssignNew(LuauTreeView, STreeView<TSharedRef<FTableViewLuauValue>>).TreeItemsSource(&LuauValues).OnGetChildren(this, &SLuauMachineDebugger::OnGetChildren).OnGenerateRow(this, &SLuauMachineDebugger::OnGenerateDebuggerRow)
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
		FLuauMachineModule::Get().OnRegisteredLuauStatesChanged.AddSP(this, &SLuauMachineDebugger::OnRegisteredLuauStatesChanged);
	}

	void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		Collector.AddReferencedObject(SelectedLuauState);
	}

#if ENGINE_MAJOR_VERSION > 4
	virtual FString GetReferencerName() const override
	{
		return TEXT("SLuauMachineDebugger");
	}
#endif

protected:
	TArray<TSharedRef<FTableViewLuauValue>> LuauValues;
	TSharedPtr<STreeView<TSharedRef<FTableViewLuauValue>>> LuauTreeView;
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 4
	TObjectPtr<ULuauState> SelectedLuauState;
#else
	ULuauState* SelectedLuauState;
#endif
	TArray<TSharedPtr<FString>> DetectedLuauStates;
	TSharedPtr<STextComboBox> LuauStatesComboBox;
	TSharedPtr<STextBlock> DebugText;
	FString DebugTextContext;
	TSharedPtr<STextBlock> ReferencersText;
	FString ReferencersTextContext;
};

TSharedRef<SDockTab> FLuauMachineEditorModule::CreateLuauMachineDebugger(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab).TabRole(ETabRole::NomadTab)
		[
			SNew(SLuauMachineDebugger)
		];
}

void FLuauMachineEditorModule::ShutdownModule()
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

FLuauMachineEditorModule& FLuauMachineEditorModule::Get()
{
	static FLuauMachineEditorModule* Singleton = nullptr;
	if (Singleton == nullptr)
	{
		Singleton = &FModuleManager::LoadModuleChecked<FLuauMachineEditorModule>("LuauMachineEditor");
	}
	return *Singleton;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FLuauMachineEditorModule, LuauMachineEditor)