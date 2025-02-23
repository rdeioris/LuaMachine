// Copyright 2018-2023 - Roberto De Ioris
// Portions Copyright 2025 - Fusion Point Studios, Inc.

#include "LuauSyntaxHighlighter.h"
#include "Runtime/Slate/Public/Framework/Text/IRun.h"
#include "Runtime/Slate/Public/Framework/Text/TextLayout.h"
#include "Runtime/Slate/Public/Framework/Text/SlateTextRun.h"

#define ADD_RULE(rule) TokenizerRules.Add(FSyntaxTokenizer::FRule(TEXT(rule)))

#define ADD_RULE_BASIC(rule) ADD_RULE(rule); BasicTokens.Add(TEXT(rule))

#define ADD_RULE_STDLIB(rule) ADD_RULE(rule); StdLibTokens.Add(TEXT(rule))

FLuauMachineSyntaxHighlighterTextLayoutMarshaller::FLuauMachineSyntaxHighlighterTextLayoutMarshaller(TSharedPtr<FSyntaxTokenizer> InTokenizer, TArray<const TCHAR *> InBasicTokens, TArray<const TCHAR *> InStdLibTokens, FLuauSyntaxTextStyle InLuauSyntaxTextStyle) :
	FSyntaxHighlighterTextLayoutMarshaller(InTokenizer),
	BasicTokens(InBasicTokens),
	StdLibTokens(InStdLibTokens),
	SyntaxTextStyle(InLuauSyntaxTextStyle)
{

}

TSharedRef<FLuauMachineSyntaxHighlighterTextLayoutMarshaller> FLuauMachineSyntaxHighlighterTextLayoutMarshaller::Create(FLuauSyntaxTextStyle LuauSyntaxTextStyle)
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
	ADD_RULE("...");
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
	ADD_RULE("if");
	ADD_RULE("or");
	ADD_RULE("until");
	ADD_RULE("while");

	ADD_RULE_BASIC("assert");
	ADD_RULE_BASIC("collectgarbage");
	ADD_RULE_BASIC("require");
	ADD_RULE_BASIC("dofile");
	ADD_RULE_BASIC("error");
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

	for (TPair<FString, FLinearColor>& Pair : LuauSyntaxTextStyle.CustomTextColorMapping)
	{
		LuauSyntaxTextStyle.CustomTextStyleMapping.Add(Pair.Key, FTextBlockStyle(LuauSyntaxTextStyle.NormalTextStyle).SetColorAndOpacity(Pair.Value));
	}

	for (TPair<FString, FTextBlockStyle>& Pair : LuauSyntaxTextStyle.CustomTextStyleMapping)
	{
		TokenizerRules.Add(FSyntaxTokenizer::FRule(*Pair.Key));
	}

	TokenizerRules.Sort([](const FSyntaxTokenizer::FRule& A, const FSyntaxTokenizer::FRule& B) {
		return A.MatchText.Len() > B.MatchText.Len();
	});


	return MakeShareable(new FLuauMachineSyntaxHighlighterTextLayoutMarshaller(FSyntaxTokenizer::Create(TokenizerRules), BasicTokens, StdLibTokens, LuauSyntaxTextStyle));
}

void FLuauMachineSyntaxHighlighterTextLayoutMarshaller::ParseTokens(const FString& SourceString, FTextLayout& TargetTextLayout, TArray<FSyntaxTokenizer::FTokenizedLine> TokenizedLines)
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

			FRunInfo RunInfo(TEXT("SyntaxHighlight.LuauMachine.Normal"));

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
							RunInfo.Name = TEXT("SyntaxHighlight.LuauMachine.Comment");
							CurrentBlockStyle = SyntaxTextStyle.CommentTextStyle;
							ParseState = EParseState::LookingForSingleLineComment;
						}
						else if (TokenString == TEXT("--[["))
						{
							RunInfo.Name = TEXT("SyntaxHighlight.LuauMachine.Comment");
							CurrentBlockStyle = SyntaxTextStyle.CommentTextStyle;
							ParseState = EParseState::LookingForMultiLineComment;
						}
						else if (TokenString == TEXT("[["))
						{
							RunInfo.Name = TEXT("SyntaxHighlight.LuauMachine.String");
							CurrentBlockStyle = SyntaxTextStyle.StringTextStyle;
							ParseState = EParseState::LookingForMultiLineString;
							bHasMatchedSyntax = true;
						}
						else if (TokenString == TEXT("'"))
						{
							RunInfo.Name = TEXT("SyntaxHighlight.LuauMachine.String");
							CurrentBlockStyle = SyntaxTextStyle.StringTextStyle;
							ParseState = EParseState::LookingForSingleQuoteString;
							bHasMatchedSyntax = true;
						}
						else if (TokenString == TEXT("\""))
						{
							RunInfo.Name = TEXT("SyntaxHighlight.LuauMachine.String");
							CurrentBlockStyle = SyntaxTextStyle.StringTextStyle;
							ParseState = EParseState::LookingForDoubleQuoteString;
							bHasMatchedSyntax = true;
						}
						else if (!TChar<WIDECHAR>::IsAlpha(NextChar) && !TChar<WIDECHAR>::IsDigit(NextChar) && !TChar<WIDECHAR>::IsAlpha(PrevChar) && !TChar<WIDECHAR>::IsDigit(PrevChar) && NextChar != TCHAR('_') && PrevChar != TCHAR('_'))
						{
							if (TokenString == TEXT("nil") || TokenString == TEXT("self") || TokenString == TEXT("_G") || TokenString == TEXT("_VERSION") || TokenString == TEXT("..."))
							{
								RunInfo.Name = TEXT("SyntaxHighlight.LuauMachine.Nil");
								CurrentBlockStyle = SyntaxTextStyle.NilTextStyle;
							}
							else if (BasicTokens.Contains(TokenString))
							{
								RunInfo.Name = TEXT("SyntaxHighlight.LuauMachine.Basic");
								CurrentBlockStyle = SyntaxTextStyle.BasicTextStyle;
							}
							else if (StdLibTokens.Contains(TokenString))
							{
								RunInfo.Name = TEXT("SyntaxHighlight.LuauMachine.StdLib");
								CurrentBlockStyle = SyntaxTextStyle.StdLibTextStyle;
							}
							else {
								if (SyntaxTextStyle.CustomTextStyleMapping.Contains(TokenString))
								{
									RunInfo.Name = TEXT("SyntaxHighlight.LuauMachine.Custom");
									CurrentBlockStyle = SyntaxTextStyle.CustomTextStyleMapping[TokenString];
								}
								else
								{
									RunInfo.Name = TEXT("SyntaxHighlight.LuauMachine.Keyword");
									CurrentBlockStyle = SyntaxTextStyle.KeywordTextStyle;
								}
							}
							ParseState = EParseState::None;
						}
					}
					else if (ParseState == EParseState::LookingForMultiLineComment && TokenString == TEXT("--]]"))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.LuauMachine.Comment");
						CurrentBlockStyle = SyntaxTextStyle.CommentTextStyle;
						ParseState = EParseState::None;
					}
					else if (ParseState == EParseState::LookingForMultiLineString && TokenString == TEXT("]]"))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.LuauMachine.String");
						CurrentBlockStyle = SyntaxTextStyle.StringTextStyle;
						ParseState = EParseState::None;
					}
					else if (ParseState == EParseState::LookingForSingleQuoteString && TokenString == TEXT("'"))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.LuauMachine.String");
						CurrentBlockStyle = SyntaxTextStyle.StringTextStyle;
						ParseState = EParseState::None;
					}
					else if (ParseState == EParseState::LookingForDoubleQuoteString && TokenString == TEXT("\""))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.LuauMachine.String");
						CurrentBlockStyle = SyntaxTextStyle.StringTextStyle;
						ParseState = EParseState::None;
					}

				}

				if (Token.Type == FSyntaxTokenizer::ETokenType::Literal || !bHasMatchedSyntax)
				{
					if (ParseState == EParseState::LookingForSingleLineComment)
					{
						RunInfo.Name = TEXT("SyntaxHighlight.LuauMachine.Comment");
						CurrentBlockStyle = SyntaxTextStyle.CommentTextStyle;
					}
					else if (ParseState == EParseState::LookingForMultiLineComment)
					{
						RunInfo.Name = TEXT("SyntaxHighlight.LuauMachine.Comment");
						CurrentBlockStyle = SyntaxTextStyle.CommentTextStyle;
					}
					else if (ParseState == EParseState::LookingForMultiLineString || ParseState == EParseState::LookingForSingleQuoteString || ParseState == EParseState::LookingForDoubleQuoteString)
					{
						RunInfo.Name = TEXT("SyntaxHighlight.LuauMachine.String");
						CurrentBlockStyle = SyntaxTextStyle.StringTextStyle;
					}
				}
				TSharedRef<ISlateRun> Run = FSlateTextRun::Create(RunInfo, ModelString, CurrentBlockStyle, ModelRange);
				Runs.Add(Run);
			}
			else
			{
				RunInfo.Name = TEXT("SyntaxHighlight.LuauMachine.WhiteSpace");
				TSharedRef<ISlateRun> Run = FSlateTextRun::Create(RunInfo, ModelString, SyntaxTextStyle.NormalTextStyle, ModelRange);
				Runs.Add(Run);
			}
		}

		LinesToAdd.Emplace(MoveTemp(ModelString), MoveTemp(Runs));
	}

	TargetTextLayout.AddLines(LinesToAdd);
}