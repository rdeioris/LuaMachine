// Copyright 2018-2023 - Roberto De Ioris
// Portions Copyright 2025 - Fusion Point Studios, Inc.

#pragma once

#include "Runtime/Slate/Public/Framework/Text/SyntaxHighlighterTextLayoutMarshaller.h"

struct FLuauSyntaxTextStyle
{
	FTextBlockStyle NormalTextStyle;
	FTextBlockStyle CommentTextStyle;
	FTextBlockStyle StringTextStyle;
	FTextBlockStyle KeywordTextStyle;
	FTextBlockStyle NilTextStyle;
	FTextBlockStyle BasicTextStyle;
	FTextBlockStyle StdLibTextStyle;
	TMap<FString, FLinearColor> CustomTextColorMapping;
	TMap<FString, FTextBlockStyle> CustomTextStyleMapping;
};


class LUAUMACHINE_API FLuauMachineSyntaxHighlighterTextLayoutMarshaller : public FSyntaxHighlighterTextLayoutMarshaller
{
public:

	FLuauMachineSyntaxHighlighterTextLayoutMarshaller(TSharedPtr<FSyntaxTokenizer> InTokenizer, TArray<const TCHAR *> InBasicTokens, TArray<const TCHAR *> InStdLibTokens, FLuauSyntaxTextStyle InLuauSyntaxTextStyle);

	static TSharedRef<FLuauMachineSyntaxHighlighterTextLayoutMarshaller> Create(FLuauSyntaxTextStyle LuauSyntaxTextStyle);

protected:
	virtual void ParseTokens(const FString& SourceString, FTextLayout& TargetTextLayout, TArray<FSyntaxTokenizer::FTokenizedLine> TokenizedLines) override;

	TArray<const TCHAR *> BasicTokens;
	TArray<const TCHAR *> StdLibTokens;

	FLuauSyntaxTextStyle SyntaxTextStyle;
};
