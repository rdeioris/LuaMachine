// Copyright 2019 - Roberto De Ioris

#include "LuaMachineEditor.h"
#include "Editor/UnrealEd/Public/Editor.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "Runtime/Projects/Public/Interfaces/IPluginManager.h"
#include "SlateCore/Public/Styling/SlateStyleRegistry.h"
#include "LuaCodeCustomization.h"
#include "LuaValueCustomization.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructure.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"
#include "Runtime/Slate/Public/Widgets/Docking/SDockTab.h"
#include "Runtime/Slate/Public/Widgets/Text/STextBlock.h"
#include "Runtime/Slate/Public/Widgets/Views/STreeView.h"
#include "Runtime/Slate/Public/Widgets/Input/SButton.h"
#include "Runtime/Slate/Public/Widgets/Input/STextComboBox.h"
#include "LuaMachine/Public/LuaMachine.h"
#include "LuaMachine/Public/LuaBlueprintFunctionLibrary.h"

#define LOCTEXT_NAMESPACE "FLuaMachineEditorModule"

void FLuaMachineEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	StyleSet = MakeShareable(new FSlateStyleSet("LuaMachineEditor"));
	StyleSet->SetContentRoot(IPluginManager::Get().FindPlugin("LuaMachine")->GetBaseDir() / "Resources");
	StyleSet->Set("ClassThumbnail.LuaCode", new FSlateImageBrush(StyleSet->RootToContentDir("Icon128.png"), FVector2D(128.0f, 128.0f)));

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

	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));

	PropertyModule.RegisterCustomPropertyTypeLayout(TEXT("LuaValue"), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FLuaValueCustomization::MakeInstance));
	PropertyModule.RegisterCustomClassLayout(TEXT("LuaCode"), FOnGetDetailCustomizationInstance::CreateStatic(&FLuaCodeCustomization::MakeInstance));

	PropertyModule.NotifyCustomizationModuleChanged();

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner("LuaMachineDebugger", FOnSpawnTab::CreateStatic(&FLuaMachineEditorModule::CreateLuaMachineDebugger))
		.SetDisplayName(LOCTEXT("LuaMachine Debugger", "LuaMachine Debugger"))
		.SetTooltipText(LOCTEXT("Open the LuaMachine Debugger", "Open the LuaMachine Debugger"))
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "DebugTools.TabIcon"))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsMiscCategory());
}

TSharedPtr<FSlateStyleSet> FLuaMachineEditorModule::GetStyleSet()
{
	return StyleSet;
}

struct FTableViewLuaValue : public TSharedFromThis<FTableViewLuaValue>
{
	FString LuaTableKey;
	FLuaValue LuaTableValue;
	bool bExpanded;
	TArray<TSharedRef<FTableViewLuaValue>> Children;
};

class SLuaMachineDebugger : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SLuaMachineDebugger)
	{}

	SLATE_END_ARGS()

		void RebuildLuaValues()
	{
		LuaValues.Empty();

		if (!SelectedLuaState)
			return;

		SelectedLuaState->PushGlobalTable();
		FLuaValue CurrentLuaTableOwner = SelectedLuaState->ToLuaValue(-1);
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

	}

	void OnNewLuaState(ULuaState* NewLuaState)
	{
		DetectedLuaStates.Empty();
		TArray<ULuaState *> States = FLuaMachineModule::Get().GetRegisteredLuaStates();
		for (ULuaState* State : States)
		{
			DetectedLuaStates.Add(MakeShared<FString>(State->GetClass()->GetName()));
		}

		if (LuaStatesComboBox.IsValid())
		{
			LuaStatesComboBox->RefreshOptions();
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
		case ELuaValueType::UObject:
			return FSlateColor(FColor::Cyan);
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
					TArray<ULuaState *> States = FLuaMachineModule::Get().GetRegisteredLuaStates();
					for (ULuaState* State : States)
					{
						if (State->GetClass()->GetName() == *SelectedText.Get())
						{
							SelectedLuaState = State;
							break;
						}
					}
				}
			}
			RebuildLuaValues();
			LuaTreeView->RequestTreeRefresh();
		}
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
		+ SHorizontalBox::Slot()
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
		OnNewLuaState(nullptr);
		RebuildLuaValues();
		ChildSlot[
			SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SAssignNew(LuaStatesComboBox, STextComboBox).OptionsSource(&DetectedLuaStates)
				]

			+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SButton).Text(FText::FromString("Refresh")).OnClicked(this, &SLuaMachineDebugger::RefreshDebugger)
				]
			+ SVerticalBox::Slot()
				[
					SAssignNew(LuaTreeView, STreeView<TSharedRef<FTableViewLuaValue>>).TreeItemsSource(&LuaValues).OnGetChildren(this, &SLuaMachineDebugger::OnGetChildren).OnGenerateRow(this, &SLuaMachineDebugger::OnGenerateDebuggerRow)
				]
		];
		FLuaMachineModule::Get().OnNewLuaState.AddSP(this, &SLuaMachineDebugger::OnNewLuaState);
	}

protected:
	TArray<TSharedRef<FTableViewLuaValue>> LuaValues;
	TSharedPtr<STreeView<TSharedRef<FTableViewLuaValue>>> LuaTreeView;
	ULuaState* SelectedLuaState;
	TArray<TSharedPtr<FString>> DetectedLuaStates;
	TSharedPtr<STextComboBox> LuaStatesComboBox;
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
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
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