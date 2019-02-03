// Copyright 2019 - Roberto De Ioris

#include "LuaMultiLineEditableTextBox.h"
#include "Runtime/Slate/Public/Widgets/Input/SMultiLineEditableTextBox.h"
#include "Runtime/Slate/Public/Widgets/Input/SEditableTextBox.h"
#include "LuaSyntaxHighlighter.h"

#define LOCTEXT_NAMESPACE "UMG"

ULuaMultiLineEditableTextBox::ULuaMultiLineEditableTextBox()
{

	SEditableTextBox::FArguments Defaults;
	WidgetStyle = *Defaults._Style;
	WidgetStyle.BackgroundColor = FSlateColor(FLinearColor::Black);
	WidgetStyle.ForegroundColor = FSlateColor(FLinearColor::White);

	TabSize = 2;

	CodeStyle = FTextBlockStyle()
		.SetFont(WidgetStyle.Font)
		.SetColorAndOpacity(FLinearColor::White)
		.SetShadowOffset(FVector2D::ZeroVector)
		.SetSelectedBackgroundColor(FSlateColor(FLinearColor::Blue))
		.SetShadowColorAndOpacity(FLinearColor::Black);

	CommentColor = FLinearColor::Green;
	StringColor = FLinearColor(FColor::Orange);
	KeywordColor = FLinearColor(FColor::Cyan);
	NilColor = FLinearColor::Red;
	StdLibColor = FLinearColor::Yellow;
	BasicColor = FLinearColor(FColor::Magenta);

}

FText ULuaMultiLineEditableTextBox::GetText() const
{
	if (EditableTextBoxPtr.IsValid())
	{
		return EditableTextBoxPtr->GetText();
	}
	return FText::GetEmpty();
}

void  ULuaMultiLineEditableTextBox::SetText(FText InText)
{
	if (EditableTextBoxPtr.IsValid())
	{
		EditableTextBoxPtr->SetText(InText);
	}
}

FReply ULuaMultiLineEditableTextBox::OnKeyChar(const FGeometry& InGeometry, const FCharacterEvent& InCharacterEvent)
{
	const TCHAR Character = InCharacterEvent.GetCharacter();
	if (Character == TEXT('\t'))
	{
		FString Spaces;
		// using Max here for avoiding negative values
		for (int32 i = 0; i < FMath::Max(TabSize, 0); i++)
			Spaces = Spaces.AppendChar(TEXT(" ")[0]);
		EditableTextBoxPtr->InsertTextAtCursor(Spaces);
		return FReply::Handled();
	}
	return EditableTextBoxPtr->SMultiLineEditableTextBox::OnKeyChar(InGeometry, InCharacterEvent);
}

#if WITH_EDITOR
const FText ULuaMultiLineEditableTextBox::GetPaletteCategory()
{
	return LOCTEXT("Lua", "Lua");
}
#endif

TSharedRef<SWidget> ULuaMultiLineEditableTextBox::RebuildWidget()
{
	FLuaSyntaxTextStyle Style;
	Style.NormalTextStyle = CodeStyle;
	Style.CommentTextStyle = FTextBlockStyle(CodeStyle).SetColorAndOpacity(CommentColor);
	Style.KeywordTextStyle = FTextBlockStyle(CodeStyle).SetColorAndOpacity(KeywordColor);
	Style.NilTextStyle = FTextBlockStyle(CodeStyle).SetColorAndOpacity(NilColor);
	Style.BasicTextStyle = FTextBlockStyle(CodeStyle).SetColorAndOpacity(BasicColor);
	Style.StdLibTextStyle = FTextBlockStyle(CodeStyle).SetColorAndOpacity(StdLibColor);
	Style.StringTextStyle = FTextBlockStyle(CodeStyle).SetColorAndOpacity(StringColor);

	EditableTextBoxPtr = SNew(SMultiLineEditableTextBox)
		.Marshaller(FLuaMachineSyntaxHighlighterTextLayoutMarshaller::Create(Style))
		.TextStyle(&CodeStyle)
		.OnKeyCharHandler_UObject(this, &ULuaMultiLineEditableTextBox::OnKeyChar)
		.Style(&WidgetStyle);
	return EditableTextBoxPtr.ToSharedRef();
}

#undef LOCTEXT_NAMESPACE