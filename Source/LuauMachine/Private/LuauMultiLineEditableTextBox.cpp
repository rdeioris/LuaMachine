// Copyright 2018-2023 - Roberto De Ioris
// Portions Copyright 2025 - Fusion Point Studios, Inc.

#include "LuauMultiLineEditableTextBox.h"
#include "Runtime/Slate/Public/Widgets/Input/SMultiLineEditableTextBox.h"
#include "Runtime/Slate/Public/Widgets/Input/SEditableTextBox.h"
#include "LuauSyntaxHighlighter.h"

#define LOCTEXT_NAMESPACE "UMG"

FLuauCustomHighlighter::FLuauCustomHighlighter()
{
	Color = FLinearColor::White;
}

ULuauMultiLineEditableTextBox::ULuauMultiLineEditableTextBox()
{
#if ENGINE_MAJOR_VERSION >=5 && ENGINE_MINOR_VERSION >= 2
	EditableTextBoxStyle.SetTextStyle(CodeStyle);
#endif

	SEditableTextBox::FArguments Defaults;
	WidgetStyle = *Defaults._Style;
	WidgetStyle.BackgroundColor = FSlateColor(FLinearColor::Black);
	WidgetStyle.ForegroundColor = FSlateColor(FLinearColor::White);

	TabSize = 2;

	bIsReadonly = false;
	bHandleTab = true;
	bHandleArrows = true;

	CodeStyle = FTextBlockStyle()
#if ENGINE_MAJOR_VERSION > 4 && ENGINE_MINOR_VERSION > 0
		.SetFont(WidgetStyle.TextStyle.Font)
#else
		.SetFont(WidgetStyle.Font)
#endif
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

FText ULuauMultiLineEditableTextBox::GetText() const
{
	if (EditableTextBoxPtr.IsValid())
	{
		return EditableTextBoxPtr->GetText();
	}
	return FText::GetEmpty();
}

FText ULuauMultiLineEditableTextBox::GetSelectedText() const
{
	if (EditableTextBoxPtr.IsValid())
	{
		return EditableTextBoxPtr->GetSelectedText();
	}
	return FText::GetEmpty();
}

void  ULuauMultiLineEditableTextBox::SetText(FText InText)
{
	if (EditableTextBoxPtr.IsValid())
	{
		EditableTextBoxPtr->SetText(InText);
	}
}

FReply ULuauMultiLineEditableTextBox::OnKeyChar(const FGeometry& InGeometry, const FCharacterEvent& InCharacterEvent)
{
	const TCHAR Character = InCharacterEvent.GetCharacter();
	if (Character == TEXT('\t'))
	{
		FString Spaces;
		// using Max here for avoiding negative values
		for (int32 i = 0; i < FMath::Max(TabSize, 0); i++)
			Spaces = Spaces.AppendChar(TEXT(" ")[0]);
		EditableTextBoxPtr->ClearSelection();
		EditableTextBoxPtr->InsertTextAtCursor(Spaces);
		return FReply::Handled();
	}
	return EditableTextBoxPtr->SMultiLineEditableTextBox::OnKeyChar(InGeometry, InCharacterEvent);
}

FReply ULuauMultiLineEditableTextBox::OnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	FKey Key = InKeyEvent.GetKey();
	if (bHandleTab && Key == EKeys::Tab)
	{
		return FReply::Handled();
	}

	if (bHandleArrows)
	{
		if (Key == EKeys::Up)
		{
			MoveCursorUp();
			return FReply::Handled();
		}

		if (Key == EKeys::Down)
		{
			MoveCursorDown();
			return FReply::Handled();
		}

		if (Key == EKeys::Right)
		{
			MoveCursorRight();
			return FReply::Handled();
		}

		if (Key == EKeys::Left)
		{
			MoveCursorLeft();
			return FReply::Handled();
		}
	}

	return EditableTextBoxPtr->SMultiLineEditableTextBox::OnKeyDown(InGeometry, InKeyEvent);
}

void ULuauMultiLineEditableTextBox::OnCursorMoved(const FTextLocation& Location)
{
	CursorLocation = Location;
}

int32 ULuauMultiLineEditableTextBox::GetCursorLine() const
{
	return CursorLocation.GetLineIndex();
}

int32 ULuauMultiLineEditableTextBox::GetCursorColumn() const
{
	return CursorLocation.GetOffset();
}

void ULuauMultiLineEditableTextBox::CursorGoTo(int32 Line, int32 Column)
{
	if (Line < 0)
		Line = 0;
	if (Column < 0)
		Column = 0;
	return EditableTextBoxPtr->GoTo(FTextLocation(Line, Column));
}

void ULuauMultiLineEditableTextBox::MoveCursorUp()
{
	return CursorGoTo(CursorLocation.GetLineIndex() - 1, 0);
}

void ULuauMultiLineEditableTextBox::MoveCursorDown()
{
	return CursorGoTo(CursorLocation.GetLineIndex() + 1, 0);
}

void ULuauMultiLineEditableTextBox::MoveCursorRight()
{
	return CursorGoTo(CursorLocation.GetLineIndex(), CursorLocation.GetOffset() + 1);
}

void ULuauMultiLineEditableTextBox::MoveCursorLeft()
{
	return CursorGoTo(CursorLocation.GetLineIndex(), CursorLocation.GetOffset() - 1);
}

void ULuauMultiLineEditableTextBox::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	EditableTextBoxPtr->SetStyle(&WidgetStyle);

	Super::SynchronizeTextLayoutProperties(*EditableTextBoxPtr);
}

#if WITH_EDITOR
const FText ULuauMultiLineEditableTextBox::GetPaletteCategory()
{
	return LOCTEXT("Luau", "Luau");
}
#endif

void ULuauMultiLineEditableTextBox::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	EditableTextBoxPtr.Reset();
}

TSharedRef<SWidget> ULuauMultiLineEditableTextBox::RebuildWidget()
{
	FLuauSyntaxTextStyle Style;
	Style.NormalTextStyle = CodeStyle;
	Style.CommentTextStyle = FTextBlockStyle(CodeStyle).SetColorAndOpacity(CommentColor);
	Style.KeywordTextStyle = FTextBlockStyle(CodeStyle).SetColorAndOpacity(KeywordColor);
	Style.NilTextStyle = FTextBlockStyle(CodeStyle).SetColorAndOpacity(NilColor);
	Style.BasicTextStyle = FTextBlockStyle(CodeStyle).SetColorAndOpacity(BasicColor);
	Style.StdLibTextStyle = FTextBlockStyle(CodeStyle).SetColorAndOpacity(StdLibColor);
	Style.StringTextStyle = FTextBlockStyle(CodeStyle).SetColorAndOpacity(StringColor);
	for (FLuauCustomHighlighter& Highlighter : CustomTokensMapping)
	{
		for (FString& Token : Highlighter.Tokens)
		{
			Style.CustomTextColorMapping.Add(Token, Highlighter.Color);
		}
	}

	EditableTextBoxPtr = SNew(SMultiLineEditableTextBox)
		.Marshaller(FLuauMachineSyntaxHighlighterTextLayoutMarshaller::Create(Style))
#if ENGINE_MAJOR_VERSION >=5 && ENGINE_MINOR_VERSION >= 1
		.Style(&EditableTextBoxStyle)
#else
		.TextStyle(&CodeStyle)
#endif
		.OnKeyCharHandler_UObject(this, &ULuauMultiLineEditableTextBox::OnKeyChar)
		.OnKeyDownHandler_UObject(this, &ULuauMultiLineEditableTextBox::OnKeyDown)
		.IsReadOnly(bIsReadonly)
		.AllowContextMenu(false)
		.OnCursorMoved_UObject(this, &ULuauMultiLineEditableTextBox::OnCursorMoved)
		.Style(&WidgetStyle);

	return EditableTextBoxPtr.ToSharedRef();
}

#undef LOCTEXT_NAMESPACE