// Copyright 2019 - Roberto De Ioris

#pragma once

#include "Runtime/Slate/Public/Framework/Text/SyntaxHighlighterTextLayoutMarshaller.h"

struct FLuaSyntaxTextStyle
{
	FTextBlockStyle NormalTextStyle;
	FTextBlockStyle CommentTextStyle;
	FTextBlockStyle StringTextStyle;
	FTextBlockStyle KeywordTextStyle;
	FTextBlockStyle NilTextStyle;
	FTextBlockStyle BasicTextStyle;
	FTextBlockStyle StdLibTextStyle;
};


class LUAMACHINE_API FLuaMachineSyntaxHighlighterTextLayoutMarshaller : public FSyntaxHighlighterTextLayoutMarshaller
{
public:

	FLuaMachineSyntaxHighlighterTextLayoutMarshaller(TSharedPtr<FSyntaxTokenizer> InTokenizer, TArray<const TCHAR *> InBasicTokens, TArray<const TCHAR *> InStdLibTokens, FLuaSyntaxTextStyle InLuaSyntaxTextStyle);

	static TSharedRef<FLuaMachineSyntaxHighlighterTextLayoutMarshaller> Create(FLuaSyntaxTextStyle LuaSyntaxTextStyle);

protected:
	virtual void ParseTokens(const FString& SourceString, FTextLayout& TargetTextLayout, TArray<FSyntaxTokenizer::FTokenizedLine> TokenizedLines) override;

	FLuaSyntaxTextStyle SyntaxTextStyle;

	TArray<const TCHAR *> BasicTokens;
	TArray<const TCHAR *> StdLibTokens;
};
