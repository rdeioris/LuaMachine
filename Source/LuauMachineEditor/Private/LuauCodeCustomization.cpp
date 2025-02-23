// Copyright 2018-2023 - Roberto De Ioris

#include "LuauCodeCustomization.h"
#include "LuauMachineEditor.h"
#include "Runtime/Slate/Public/Widgets/Input/SMultiLineEditableTextBox.h"
#include "Editor/PropertyEditor/Public/PropertyHandle.h"
#include "Editor/PropertyEditor/Public/DetailLayoutBuilder.h"
#include "Editor/PropertyEditor/Public/DetailCategoryBuilder.h"
#include "Editor/PropertyEditor/Public/IDetailChildrenBuilder.h"
#include "Editor/PropertyEditor/Public/DetailWidgetRow.h"
#include "Runtime/Slate/Public/Widgets/Text/STextBlock.h"
#include "Runtime/SlateCore/Public/Brushes/SlateColorBrush.h"
#include "Runtime/Slate/Public/Widgets/Text/SlateEditableTextLayout.h"
#include "Runtime/SlateCore/Public/Fonts/FontMeasure.h"
#include "Runtime/Slate/Public/Framework/Application/SlateApplication.h"
#include "Runtime/Slate/Public/Framework/MultiBox/MultiBoxBuilder.h"
#include "LuauMachine/Public/LuauCode.h"
#include "LuauMachine/Public/LuauSyntaxHighlighter.h"


class SLuauMultiLineEditableTextBox : public SMultiLineEditableTextBox
{
public:
	SLATE_BEGIN_ARGS(SLuauMultiLineEditableTextBox) {}

	SLATE_ARGUMENT(TWeakObjectPtr<ULuauCode>, LuauCodeOwner);

	SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
	{
		LuauCode = InArgs._LuauCodeOwner;
		CurrentLine = 1;
		CurrentColumn = 1;

		FLuauSyntaxTextStyle LuauTextStyle;

		LuauTextStyle.NormalTextStyle = FLuauMachineEditorModule::Get().GetStyleSet()->GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.LuauMachine.Normal");
		LuauTextStyle.CommentTextStyle = FLuauMachineEditorModule::Get().GetStyleSet()->GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.LuauMachine.Comment");
		LuauTextStyle.StringTextStyle = FLuauMachineEditorModule::Get().GetStyleSet()->GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.LuauMachine.String");
		LuauTextStyle.KeywordTextStyle = FLuauMachineEditorModule::Get().GetStyleSet()->GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.LuauMachine.Keyword");
		LuauTextStyle.NilTextStyle = FLuauMachineEditorModule::Get().GetStyleSet()->GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.LuauMachine.Nil");
		LuauTextStyle.BasicTextStyle = FLuauMachineEditorModule::Get().GetStyleSet()->GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.LuauMachine.Basic");
		LuauTextStyle.StdLibTextStyle = FLuauMachineEditorModule::Get().GetStyleSet()->GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.LuauMachine.StdLib");

		SyntaxHighlighter = FLuauMachineSyntaxHighlighterTextLayoutMarshaller::Create(LuauTextStyle);

		FMenuExtensionDelegate Delegate;
		Delegate.BindLambda([&](FMenuBuilder &Builder)
		{
			Builder.AddWidget(SNew(STextBlock).Text_Lambda([&]() {
				return FText::FromString(FString::Printf(TEXT("Line: %d Column: %d"), CurrentLine, CurrentColumn));
			}), FText::GetEmpty());

		});

		SMultiLineEditableTextBox::Construct(
			SMultiLineEditableTextBox::FArguments()
			.AutoWrapText(false)
			.Margin(0.0f)
			.Text(LuauCode->Code)
			.Marshaller(SyntaxHighlighter)
			.OnTextChanged(this, &SLuauMultiLineEditableTextBox::UpdateLuauCode)
			.BackgroundColor(FSlateColor(FLinearColor::Black))
			.ForegroundColor(FSlateColor(FLinearColor::White))
			.ContextMenuExtender(Delegate)
			.OnCursorMoved_Lambda([&](const FTextLocation& Location) {
			CurrentLine = Location.GetLineIndex() + 1;
			CurrentColumn = Location.GetOffset() + 1;
		})
		);

	}

	void UpdateLuauCode(const FText& InCode)
	{
		LuauCode->Code = InCode;
		LuauCode->MarkPackageDirty();
	}

protected:

	int32 CurrentLine;
	int32 CurrentColumn;

	float GetFontHeight(const FSlateFontInfo& FontInfo) const
	{
		const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		return FontMeasure->GetMaxCharacterHeight(FontInfo);
	}

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override
	{
		float MinimumHeight = 500;
		float MaxHeight = 800;

		FVector2D Size = SMultiLineEditableTextBox::ComputeDesiredSize(LayoutScaleMultiplier);
		if (Size.Y < MinimumHeight)
			Size.Y = MinimumHeight;
		if (Size.Y > MaxHeight)
			Size.Y = MaxHeight;
		return Size;
	}

	virtual FReply OnKeyChar(const FGeometry& InGeometry, const FCharacterEvent& InCharacterEvent) override
	{
		const TCHAR Character = InCharacterEvent.GetCharacter();
		if (Character == TEXT('\t'))
		{
			ClearSelection();
			InsertTextAtCursor(FString("  "));
			return FReply::Handled();
		}
		return SMultiLineEditableTextBox::OnKeyChar(InGeometry, InCharacterEvent);
	}

	virtual FReply OnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override
	{

		if (InKeyEvent.GetKeyCode() == 9)
		{
			return FReply::Handled();
		}
		return SMultiLineEditableTextBox::OnKeyDown(InGeometry, InKeyEvent);
	}

private:
	TWeakObjectPtr<ULuauCode> LuauCode;
	TSharedPtr<FLuauMachineSyntaxHighlighterTextLayoutMarshaller> SyntaxHighlighter;
};

class SLuauEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLuauEditor) {}

	SLATE_ARGUMENT(TWeakObjectPtr<ULuauCode>, LuauCodeOwner);

	SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
	{
		LuauCode = InArgs._LuauCodeOwner;

		BackgroundColor = FSlateColorBrush(FLinearColor::Black);

		ChildSlot.Padding(4)[
			SNew(SBorder).BorderImage(&BackgroundColor).BorderBackgroundColor(FSlateColor(FLinearColor::White))
				[

					SNew(SLuauMultiLineEditableTextBox)
					.LuauCodeOwner(LuauCode)

				]
		];
	}



private:
	TWeakObjectPtr<ULuauCode> LuauCode;
	FSlateBrush BackgroundColor;
};

void FLuauCodeCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);

	if (Objects.Num() != 1)
	{
		return;
	}

	TWeakObjectPtr<ULuauCode> LuauCode = Cast<ULuauCode>(Objects[0].Get());

	TSharedRef<IPropertyHandle> Prop = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(ULuauCode, Code));
	DetailBuilder.HideProperty(Prop);


	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("Code");
	Category.AddCustomRow(FText::FromString("Code")).WholeRowContent()[

		SNew(SLuauEditor).LuauCodeOwner(LuauCode)

	];

}




