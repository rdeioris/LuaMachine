// Copyright 2025 - Roberto De Ioris.


#include "LuaMachineK2Node_MakeLuaCodeString.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "Kismet/KismetSystemLibrary.h"
#include "KismetCompiler.h"
#include "KismetNodes/SGraphNodeK2Base.h"
#include "K2Node_CallFunction.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "LuaSyntaxHighlighter.h"

FText ULuaMachineK2Node_MakeLuaCodeString::GetMenuCategory() const
{
	return FText::FromString("LuaMachine");
}

void ULuaMachineK2Node_MakeLuaCodeString::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	Super::GetMenuActions(ActionRegistrar);
	UClass* Action = GetClass();

	if (ActionRegistrar.IsOpenForRegistration(Action)) {
		UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());
		check(Spawner != nullptr);
		ActionRegistrar.AddBlueprintAction(Action, Spawner);
	}
}

void ULuaMachineK2Node_MakeLuaCodeString::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_String, NAME_None);
}

FText ULuaMachineK2Node_MakeLuaCodeString::GetNodeTitle(ENodeTitleType::Type Title) const
{
	return FText::FromString(TEXT("Make LuaCode String"));
}

FText ULuaMachineK2Node_MakeLuaCodeString::GetTooltipText() const
{
	return FText::FromString(TEXT("Create LuaCode String with syntax highlighting"));
}

FLinearColor ULuaMachineK2Node_MakeLuaCodeString::GetNodeTitleColor() const
{
	return FLinearColor(1, 0, 1);
}

const FString& ULuaMachineK2Node_MakeLuaCodeString::GetLuaCodeString() const
{
	return LuaCodeString;
}

TSharedPtr<SGraphNode> ULuaMachineK2Node_MakeLuaCodeString::CreateVisualWidget()
{
	class SCompushadyShaderNodeWidget : public SGraphNodeK2Base
	{
	public:
		SLATE_BEGIN_ARGS(SCompushadyShaderNodeWidget)
			{
			}

		SLATE_END_ARGS()

		/** Constructs this widget with InArgs */
		void Construct(const FArguments& InArgs, UK2Node* InNode)
		{
			GraphNode = InNode;
			UpdateGraphNode();
		}

		void CreatePinWidgets() override
		{
			SGraphNodeK2Base::CreatePinWidgets();

			ULuaMachineK2Node_MakeLuaCodeString* MakeString = Cast<ULuaMachineK2Node_MakeLuaCodeString>(GraphNode);

			LeftNodeBox->AddSlot()
				.AutoHeight()
				[
					SNew(SBox).MinDesiredWidth(500).MinDesiredHeight(350).MaxDesiredWidth(1000).MaxDesiredHeight(700)
						[
							SAssignNew(MakeString->LuaCodeStringEditorWidget, SMultiLineEditableTextBox).Marshaller(MakeString->GetSyntaxHighlighter())
								.Text(FText::FromString(MakeString->GetLuaCodeString()))
								.OnTextChanged_UObject(MakeString, &ULuaMachineK2Node_MakeLuaCodeString::UpdateLuaCodeString)
								.OnKeyCharHandler_UObject(MakeString, &ULuaMachineK2Node_MakeLuaCodeString::OnKeyChar)
						]
				];
		}
	};

	return SNew(SCompushadyShaderNodeWidget, this);
}

void ULuaMachineK2Node_MakeLuaCodeString::UpdateLuaCodeString(const FText& NewText)
{
	Modify();
	LuaCodeString = NewText.ToString();
	if (HasValidBlueprint())
	{
		UBlueprint* BP = GetBlueprint();
		BP->Status = BS_Dirty;
	}
}

FReply ULuaMachineK2Node_MakeLuaCodeString::OnKeyChar(const FGeometry& InGeometry, const FCharacterEvent& InCharacterEvent)
{
	const TCHAR Character = InCharacterEvent.GetCharacter();
	if (Character == TEXT('\t'))
	{
		FString Spaces = TEXT("    ");
		LuaCodeStringEditorWidget->ClearSelection();
		LuaCodeStringEditorWidget->InsertTextAtCursor(Spaces);
		return FReply::Handled();
	}
	return LuaCodeStringEditorWidget->OnKeyChar(InGeometry, InCharacterEvent);
}

void ULuaMachineK2Node_MakeLuaCodeString::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	const FName FunctionName = GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, MakeLiteralString);
	UK2Node_CallFunction* MakeLiteralStringNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	MakeLiteralStringNode->SetFromFunction(UKismetSystemLibrary::StaticClass()->FindFunctionByName(FunctionName));
	MakeLiteralStringNode->AllocateDefaultPins();

	UEdGraphPin* NewInputPin = MakeLiteralStringNode->FindPinChecked(TEXT("Value"));

	NewInputPin->DefaultValue = LuaCodeString;

	UEdGraphPin* OrgReturnPin = Pins[0];
	UEdGraphPin* MakeLiteralStringReturnPin = MakeLiteralStringNode->FindPinChecked(UEdGraphSchema_K2::PN_ReturnValue);
	CompilerContext.MovePinLinksToIntermediate(*OrgReturnPin, *MakeLiteralStringReturnPin);

	BreakAllNodeLinks();
}

TSharedPtr<ITextLayoutMarshaller> ULuaMachineK2Node_MakeLuaCodeString::GetSyntaxHighlighter() const
{
	FLuaSyntaxTextStyle Style;

	const FSlateFontInfo Consolas9 = FCoreStyle::GetDefaultFontStyle("Mono", 9);

	const FTextBlockStyle CodeBaseStyle = FTextBlockStyle()
		.SetFont(Consolas9)
		.SetColorAndOpacity(FLinearColor::White)
		.SetShadowOffset(FVector2D::ZeroVector)
		.SetShadowColorAndOpacity(FLinearColor::Black);

	Style.NormalTextStyle = CodeBaseStyle;
	Style.CommentTextStyle = FTextBlockStyle(CodeBaseStyle).SetColorAndOpacity(FLinearColor::Green);
	Style.KeywordTextStyle = FTextBlockStyle(CodeBaseStyle).SetColorAndOpacity(FLinearColor(FColor::Cyan));
	Style.NilTextStyle = FTextBlockStyle(CodeBaseStyle).SetColorAndOpacity(FLinearColor::Red);
	Style.BasicTextStyle = FTextBlockStyle(CodeBaseStyle).SetColorAndOpacity(FLinearColor::FromSRGBColor(FColor::Magenta));
	Style.StdLibTextStyle = FTextBlockStyle(CodeBaseStyle).SetColorAndOpacity(FLinearColor::Yellow);
	Style.StringTextStyle = FTextBlockStyle(CodeBaseStyle).SetColorAndOpacity(FLinearColor(FColor::Orange));
	return FLuaMachineSyntaxHighlighterTextLayoutMarshaller::Create(Style);
}
