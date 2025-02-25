// Copyright 2018-2023 - Roberto De Ioris

#include "LuaCodeCustomization.h"
#include "LuaMachineEditor.h"
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
#include "LuaMachine/Public/LuaCode.h"
#include "LuaMachine/Public/LuaSyntaxHighlighter.h"


class SLuaMultiLineEditableTextBox : public SMultiLineEditableTextBox
{
public:
	SLATE_BEGIN_ARGS(SLuaMultiLineEditableTextBox) {}

	SLATE_ARGUMENT(TWeakObjectPtr<ULuaCode>, LuaCodeOwner);

	SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
	{
		LuaCode = InArgs._LuaCodeOwner;
		CurrentLine = 1;
		CurrentColumn = 1;

		FLuaSyntaxTextStyle LuaTextStyle;

		LuaTextStyle.NormalTextStyle = FLuaMachineEditorModule::Get().GetStyleSet()->GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.LuaMachine.Normal");
		LuaTextStyle.CommentTextStyle = FLuaMachineEditorModule::Get().GetStyleSet()->GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.LuaMachine.Comment");
		LuaTextStyle.StringTextStyle = FLuaMachineEditorModule::Get().GetStyleSet()->GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.LuaMachine.String");
		LuaTextStyle.KeywordTextStyle = FLuaMachineEditorModule::Get().GetStyleSet()->GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.LuaMachine.Keyword");
		LuaTextStyle.NilTextStyle = FLuaMachineEditorModule::Get().GetStyleSet()->GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.LuaMachine.Nil");
		LuaTextStyle.BasicTextStyle = FLuaMachineEditorModule::Get().GetStyleSet()->GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.LuaMachine.Basic");
		LuaTextStyle.StdLibTextStyle = FLuaMachineEditorModule::Get().GetStyleSet()->GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.LuaMachine.StdLib");

		SyntaxHighlighter = FLuaMachineSyntaxHighlighterTextLayoutMarshaller::Create(LuaTextStyle);

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
			.Text(LuaCode->Code)
			.Marshaller(SyntaxHighlighter)
			.OnTextChanged(this, &SLuaMultiLineEditableTextBox::UpdateLuaCode)
			.BackgroundColor(FSlateColor(FLinearColor::Black))
			.ForegroundColor(FSlateColor(FLinearColor::White))
			.ContextMenuExtender(Delegate)
			.OnCursorMoved_Lambda([&](const FTextLocation& Location) {
			CurrentLine = Location.GetLineIndex() + 1;
			CurrentColumn = Location.GetOffset() + 1;
		})
		);

	}

	void UpdateLuaCode(const FText& InCode)
	{
		LuaCode->Code = InCode;
		LuaCode->MarkPackageDirty();
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
	TWeakObjectPtr<ULuaCode> LuaCode;
	TSharedPtr<FLuaMachineSyntaxHighlighterTextLayoutMarshaller> SyntaxHighlighter;
};

class SLuaEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLuaEditor) {}

	SLATE_ARGUMENT(TWeakObjectPtr<ULuaCode>, LuaCodeOwner);

	SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
	{
		LuaCode = InArgs._LuaCodeOwner;

		BackgroundColor = FSlateColorBrush(FLinearColor::Black);

		ChildSlot.Padding(4)[
			SNew(SBorder).BorderImage(&BackgroundColor).BorderBackgroundColor(FSlateColor(FLinearColor::White))
				[

					SNew(SLuaMultiLineEditableTextBox)
					.LuaCodeOwner(LuaCode)

				]
		];
	}



private:
	TWeakObjectPtr<ULuaCode> LuaCode;
	FSlateBrush BackgroundColor;
};

void FLuaCodeCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);

	if (Objects.Num() != 1)
	{
		return;
	}

	TWeakObjectPtr<ULuaCode> LuaCode = Cast<ULuaCode>(Objects[0].Get());

	TSharedRef<IPropertyHandle> Prop = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(ULuaCode, Code));
	DetailBuilder.HideProperty(Prop);


	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("Code");
	Category.AddCustomRow(FText::FromString("Code")).WholeRowContent()[

		SNew(SLuaEditor).LuaCodeOwner(LuaCode)

	];

}




