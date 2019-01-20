// Copyright 2018 - Roberto De Ioris

#include "LuaCodeCustomization.h"
#include "LuaMachineEditor.h"
#include "Runtime/SlateCore/Public/Widgets/SBoxPanel.h"
#include "Runtime/Slate/Public/Widgets/Layout/SScaleBox.h"
#include "Runtime/Slate/Public/Widgets/Input/SMultiLineEditableTextBox.h"
#include "Editor/PropertyEditor/Public/PropertyHandle.h"
#include "Editor/PropertyEditor/Public/DetailLayoutBuilder.h"
#include "Editor/PropertyEditor/Public/DetailCategoryBuilder.h"
#include "Editor/PropertyEditor/Public/IDetailChildrenBuilder.h"
#include "Editor/PropertyEditor/Public/DetailWidgetRow.h"
#include "Runtime/Slate/Public/Widgets/Text/STextBlock.h"
#include "Runtime/Slate/Public/Widgets/Layout/SGridPanel.h"
#include "Runtime/SlateCore/Public/Brushes/SlateColorBrush.h"
#include "Runtime/Slate/Public/Widgets/Text/SlateEditableTextLayout.h"
#include "Runtime/SlateCore/Public/Fonts/FontMeasure.h"
#include "Runtime/Slate/Public/Framework/Application/SlateApplication.h"
#include "Runtime/Slate/Public/Framework/Text/SyntaxHighlighterTextLayoutMarshaller.h"
#include "Runtime/Slate/Public/Framework/Text/SyntaxTokenizer.h"
#include "Runtime/Slate/Public/Framework/Text/TextLayout.h"
#include "Runtime/Slate/Public/Framework/Text/SlateTextRun.h"
#include "LuaMachine/Public/LuaCode.h"

#define ADD_RULE(rule) TokenizerRules.Add(FSyntaxTokenizer::FRule(TEXT(rule)))

#define ADD_RULE_BASIC(rule) ADD_RULE(rule); BasicTokens.Add(TEXT(rule))

#define ADD_RULE_STDLIB(rule) ADD_RULE(rule); StdLibTokens.Add(TEXT(rule))

class FLuaMachineSyntaxHighlighterTextLayoutMarshaller : public FSyntaxHighlighterTextLayoutMarshaller
{
public:
	struct FSyntaxTextStyle
	{
		FSyntaxTextStyle() :
			NormalTextStyle(FLuaMachineEditorModule::Get().GetStyleSet()->GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.LuaMachine.Normal")),
			CommentTextStyle(FLuaMachineEditorModule::Get().GetStyleSet()->GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.LuaMachine.Comment")),
			StringTextStyle(FLuaMachineEditorModule::Get().GetStyleSet()->GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.LuaMachine.String")),
			KeywordTextStyle(FLuaMachineEditorModule::Get().GetStyleSet()->GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.LuaMachine.Keyword")),
			NilTextStyle(FLuaMachineEditorModule::Get().GetStyleSet()->GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.LuaMachine.Nil")),
			BasicTextStyle(FLuaMachineEditorModule::Get().GetStyleSet()->GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.LuaMachine.Basic")),
			StdLibTextStyle(FLuaMachineEditorModule::Get().GetStyleSet()->GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.LuaMachine.StdLib"))
		{}
		FTextBlockStyle NormalTextStyle;
		FTextBlockStyle CommentTextStyle;
		FTextBlockStyle StringTextStyle;
		FTextBlockStyle KeywordTextStyle;
		FTextBlockStyle NilTextStyle;
		FTextBlockStyle BasicTextStyle;
		FTextBlockStyle StdLibTextStyle;
	};

	FLuaMachineSyntaxHighlighterTextLayoutMarshaller(TSharedPtr<FSyntaxTokenizer> InTokenizer, TArray<const TCHAR *> InBasicTokens, TArray<const TCHAR *> InStdLibTokens, const FSyntaxTextStyle& InSyntaxTextStyle) : FSyntaxHighlighterTextLayoutMarshaller(InTokenizer), SyntaxTextStyle(InSyntaxTextStyle), BasicTokens(InBasicTokens), StdLibTokens(InStdLibTokens)
	{

	}


	static TSharedRef< FLuaMachineSyntaxHighlighterTextLayoutMarshaller> Create()
	{
		TArray<FSyntaxTokenizer::FRule> TokenizerRules;

		TArray<const TCHAR *> BasicTokens;
		TArray<const TCHAR *> StdLibTokens;

		ADD_RULE("--[[");
		ADD_RULE("--]]");
		ADD_RULE("--");
		ADD_RULE("[[");
		ADD_RULE("]]");
		ADD_RULE("\\'");
		ADD_RULE("\\\"");
		ADD_RULE("'");
		ADD_RULE("\"");

		ADD_RULE("nil");
		ADD_RULE("self");
		ADD_RULE("arg");
		ADD_RULE("_G");
		ADD_RULE("_VERSION");

		ADD_RULE("and");
		ADD_RULE("end");
		ADD_RULE("in");
		ADD_RULE("repeat");
		ADD_RULE("break");
		ADD_RULE("false");
		ADD_RULE("local");
		ADD_RULE("return");
		ADD_RULE("do");
		ADD_RULE("for");
		ADD_RULE("then");
		ADD_RULE("else");
		ADD_RULE("elseif");
		ADD_RULE("function");
		ADD_RULE("not");
		ADD_RULE("true");
		ADD_RULE("elseif");
		ADD_RULE("if");
		ADD_RULE("or");
		ADD_RULE("until");
		ADD_RULE("while");

		ADD_RULE_BASIC("assert");
		ADD_RULE_BASIC("collectgarbage");
		ADD_RULE_BASIC("require");
		ADD_RULE_BASIC("dofile");
		ADD_RULE_BASIC("error ");
		ADD_RULE_BASIC("getmetatable");
		ADD_RULE_BASIC("ipairs");
		ADD_RULE_BASIC("load");
		ADD_RULE_BASIC("loadfile");
		ADD_RULE_BASIC("next");
		ADD_RULE_BASIC("pairs");
		ADD_RULE_BASIC("pcall");
		ADD_RULE_BASIC("print");
		ADD_RULE_BASIC("rawequal");
		ADD_RULE_BASIC("rawget");
		ADD_RULE_BASIC("rawlen");
		ADD_RULE_BASIC("rawset");
		ADD_RULE_BASIC("select");
		ADD_RULE_BASIC("setmetatable");
		ADD_RULE_BASIC("tonumber");
		ADD_RULE_BASIC("tostring");
		ADD_RULE_BASIC("type");
		ADD_RULE_BASIC("xpcall");

		ADD_RULE_STDLIB("coroutine");
		ADD_RULE_STDLIB("package");
		ADD_RULE_STDLIB("string");
		ADD_RULE_STDLIB("utf8");
		ADD_RULE_STDLIB("table");
		ADD_RULE_STDLIB("math");
		ADD_RULE_STDLIB("io");
		ADD_RULE_STDLIB("file");
		ADD_RULE_STDLIB("os");
		ADD_RULE_STDLIB("debug");


		return MakeShareable(new FLuaMachineSyntaxHighlighterTextLayoutMarshaller(FSyntaxTokenizer::Create(TokenizerRules), BasicTokens, StdLibTokens, FSyntaxTextStyle()));
	}
protected:
	virtual void ParseTokens(const FString& SourceString, FTextLayout& TargetTextLayout, TArray<FSyntaxTokenizer::FTokenizedLine> TokenizedLines) override
	{

		enum class EParseState : uint8
		{
			None,
			LookingForSingleLineComment,
			LookingForMultiLineComment,
			LookingForSingleQuoteString,
			LookingForDoubleQuoteString,
			LookingForMultiLineString,
		};

		TArray<FTextLayout::FNewLineData> LinesToAdd;
		LinesToAdd.Reserve(TokenizedLines.Num());


		EParseState ParseState = EParseState::None;

		for (const FSyntaxTokenizer::FTokenizedLine& TokenizedLine : TokenizedLines)
		{
			TSharedRef<FString> ModelString = MakeShareable(new FString());
			TArray<TSharedRef<IRun>> Runs;

			if (ParseState == EParseState::LookingForSingleLineComment)
			{
				ParseState = EParseState::None;
			}



			for (const FSyntaxTokenizer::FToken& Token : TokenizedLine.Tokens)
			{
				const FString TokenString = SourceString.Mid(Token.Range.BeginIndex, Token.Range.Len());
				const FTextRange ModelRange(ModelString->Len(), ModelString->Len() + TokenString.Len());

				ModelString->Append(TokenString);

				FRunInfo RunInfo(TEXT("SyntaxHighlight.LuaMachine.Normal"));

				FTextBlockStyle CurrentBlockStyle = SyntaxTextStyle.NormalTextStyle;

				bool bIsWhitespace = FString(TokenString).TrimEnd().IsEmpty();
				if (!bIsWhitespace)
				{
					bool bHasMatchedSyntax = false;
					if (Token.Type == FSyntaxTokenizer::ETokenType::Syntax)
					{
						if (ParseState == EParseState::None)
						{
							TCHAR NextChar = TEXT(" ")[0];
							TCHAR PrevChar = TEXT(" ")[0];
							if (Token.Range.EndIndex < SourceString.Len())
							{
								NextChar = SourceString[Token.Range.EndIndex];
							}
							if (Token.Range.BeginIndex > 0)
							{
								PrevChar = SourceString[Token.Range.BeginIndex - 1];
							}
							if (TokenString == TEXT("--"))
							{
								RunInfo.Name = TEXT("SyntaxHighlight.LuaMachine.Comment");
								CurrentBlockStyle = SyntaxTextStyle.CommentTextStyle;
								ParseState = EParseState::LookingForSingleLineComment;
							}
							else if (TokenString == TEXT("--[["))
							{
								RunInfo.Name = TEXT("SyntaxHighlight.LuaMachine.Comment");
								CurrentBlockStyle = SyntaxTextStyle.CommentTextStyle;
								ParseState = EParseState::LookingForMultiLineComment;
							}
							else if (TokenString == TEXT("[["))
							{
								RunInfo.Name = TEXT("SyntaxHighlight.LuaMachine.String");
								CurrentBlockStyle = SyntaxTextStyle.StringTextStyle;
								ParseState = EParseState::LookingForMultiLineString;
								bHasMatchedSyntax = true;
							}
							else if (TokenString == TEXT("'"))
							{
								RunInfo.Name = TEXT("SyntaxHighlight.LuaMachine.String");
								CurrentBlockStyle = SyntaxTextStyle.StringTextStyle;
								ParseState = EParseState::LookingForSingleQuoteString;
								bHasMatchedSyntax = true;
							}
							else if (TokenString == TEXT("\""))
							{
								RunInfo.Name = TEXT("SyntaxHighlight.LuaMachine.String");
								CurrentBlockStyle = SyntaxTextStyle.StringTextStyle;
								ParseState = EParseState::LookingForDoubleQuoteString;
								bHasMatchedSyntax = true;
							}
							else if (!TChar<WIDECHAR>::IsAlpha(NextChar) && !TChar<WIDECHAR>::IsDigit(NextChar) && !TChar<WIDECHAR>::IsAlpha(PrevChar) && !TChar<WIDECHAR>::IsDigit(PrevChar) && NextChar != TCHAR('_') && PrevChar != TCHAR('_'))
							{
								if (TokenString == TEXT("nil") || TokenString == TEXT("self") || TokenString == TEXT("_G") || TokenString == TEXT("_VERSION") || TokenString == TEXT("arg"))
								{
									RunInfo.Name = TEXT("SyntaxHighlight.LuaMachine.Nil");
									CurrentBlockStyle = SyntaxTextStyle.NilTextStyle;
								}
								else if (BasicTokens.Contains(TokenString))
								{
									RunInfo.Name = TEXT("SyntaxHighlight.LuaMachine.Basic");
									CurrentBlockStyle = SyntaxTextStyle.BasicTextStyle;
								}
								else if (StdLibTokens.Contains(TokenString))
								{
									RunInfo.Name = TEXT("SyntaxHighlight.LuaMachine.StdLib");
									CurrentBlockStyle = SyntaxTextStyle.StdLibTextStyle;
								}
								else {
									RunInfo.Name = TEXT("SyntaxHighlight.LuaMachine.Keyword");
									CurrentBlockStyle = SyntaxTextStyle.KeywordTextStyle;
								}
								ParseState = EParseState::None;
							}
						}
						else if (ParseState == EParseState::LookingForMultiLineComment && TokenString == TEXT("--]]"))
						{
							RunInfo.Name = TEXT("SyntaxHighlight.LuaMachine.Comment");
							CurrentBlockStyle = SyntaxTextStyle.CommentTextStyle;
							ParseState = EParseState::None;
						}
						else if (ParseState == EParseState::LookingForMultiLineString && TokenString == TEXT("]]"))
						{
							RunInfo.Name = TEXT("SyntaxHighlight.LuaMachine.String");
							CurrentBlockStyle = SyntaxTextStyle.StringTextStyle;
							ParseState = EParseState::None;
						}
						else if (ParseState == EParseState::LookingForSingleQuoteString && TokenString == TEXT("'"))
						{
							RunInfo.Name = TEXT("SyntaxHighlight.LuaMachine.String");
							CurrentBlockStyle = SyntaxTextStyle.StringTextStyle;
							ParseState = EParseState::None;
						}
						else if (ParseState == EParseState::LookingForDoubleQuoteString && TokenString == TEXT("\""))
						{
							RunInfo.Name = TEXT("SyntaxHighlight.LuaMachine.String");
							CurrentBlockStyle = SyntaxTextStyle.StringTextStyle;
							ParseState = EParseState::None;
						}

					}

					if (Token.Type == FSyntaxTokenizer::ETokenType::Literal || !bHasMatchedSyntax)
					{
						if (ParseState == EParseState::LookingForSingleLineComment)
						{
							RunInfo.Name = TEXT("SyntaxHighlight.LuaMachine.Comment");
							CurrentBlockStyle = SyntaxTextStyle.CommentTextStyle;
						}
						else if (ParseState == EParseState::LookingForMultiLineComment)
						{
							RunInfo.Name = TEXT("SyntaxHighlight.LuaMachine.Comment");
							CurrentBlockStyle = SyntaxTextStyle.CommentTextStyle;
						}
						else if (ParseState == EParseState::LookingForMultiLineString || ParseState == EParseState::LookingForSingleQuoteString || ParseState == EParseState::LookingForDoubleQuoteString)
						{
							RunInfo.Name = TEXT("SyntaxHighlight.LuaMachine.String");
							CurrentBlockStyle = SyntaxTextStyle.StringTextStyle;
						}
					}
					TSharedRef<ISlateRun> Run = FSlateTextRun::Create(RunInfo, ModelString, CurrentBlockStyle, ModelRange);
					Runs.Add(Run);
				}
				else
				{
					RunInfo.Name = TEXT("SyntaxHighlight.LuaMachine.WhiteSpace");
					TSharedRef<ISlateRun> Run = FSlateTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.NormalTextStyle, ModelRange);
					Runs.Add(Run);
				}
			}

			LinesToAdd.Emplace(MoveTemp(ModelString), MoveTemp(Runs));
		}

		TargetTextLayout.AddLines(LinesToAdd);
	}

	FSyntaxTextStyle SyntaxTextStyle;

	TArray<const TCHAR *> BasicTokens;
	TArray<const TCHAR *> StdLibTokens;
};

class SLuaMultiLineEditableTextBox : public SMultiLineEditableTextBox
{
public:
	SLATE_BEGIN_ARGS(SLuaMultiLineEditableTextBox) {}

	SLATE_ARGUMENT(TWeakObjectPtr<ULuaCode>, LuaCodeOwner);

	SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
	{
		LuaCode = InArgs._LuaCodeOwner;

		SyntaxHighlighter = FLuaMachineSyntaxHighlighterTextLayoutMarshaller::Create();

		SMultiLineEditableTextBox::Construct(
			SMultiLineEditableTextBox::FArguments()
			.AutoWrapText(false)
			.Margin(0.0f)
			.Text(LuaCode->Code)
			.Marshaller(SyntaxHighlighter)
			.OnTextChanged(this, &SLuaMultiLineEditableTextBox::UpdateLuaCode)
			.BackgroundColor(FSlateColor(FLinearColor::Black))
			.ForegroundColor(FSlateColor(FLinearColor::White))
		);

	}

	void UpdateLuaCode(const FText& InCode)
	{
		LuaCode->Code = InCode;
		LuaCode->MarkPackageDirty();
	}

protected:

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

		ChildSlot.Padding(4) [
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




