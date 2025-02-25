// Copyright 2018-2023 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "Components/TextWidgetTypes.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Runtime/Launch/Resources/Version.h"
#include "LuaMultiLineEditableTextBox.generated.h"

USTRUCT(BlueprintType)
struct LUAMACHINE_API FLuaCustomHighlighter
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Lua")
	FLinearColor Color;

	UPROPERTY(EditAnywhere, Category = "Lua")
	TArray<FString> Tokens;

	FLuaCustomHighlighter();
};

/**
 * 
 */
UCLASS(meta=(DisplayName="Lua Code Box"))
class LUAMACHINE_API ULuaMultiLineEditableTextBox : public UTextLayoutWidget
{
	GENERATED_BODY()

public:
	ULuaMultiLineEditableTextBox();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style", meta = (DisplayName = "Style"))
	FEditableTextBoxStyle WidgetStyle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style", meta = (DisplayName = "Code Style"))
	FTextBlockStyle CodeStyle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style", meta = (DisplayName = "Comment Color"))
	FLinearColor CommentColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style", meta = (DisplayName = "String Color"))
	FLinearColor StringColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style", meta = (DisplayName = "Keyword Color"))
	FLinearColor KeywordColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style", meta = (DisplayName = "Nil Color"))
	FLinearColor NilColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style", meta = (DisplayName = "Basic Color"))
	FLinearColor BasicColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style", meta = (DisplayName = "StdLib Color"))
	FLinearColor StdLibColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lua Code", meta = (DisplayName = "Tab Size"))
	int32 TabSize;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	bool bIsReadonly;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lua Code")
	bool bHandleTab;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lua Code")
	bool bHandleArrows;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style")
	TArray<FLuaCustomHighlighter> CustomTokensMapping;

	UFUNCTION(BlueprintPure, Category = "Widget", meta = (DisplayName = "GetText (Lua Code Box)"))
	FText GetText() const;

	UFUNCTION(BlueprintCallable, Category = "Widget", meta = (DisplayName = "SetText (Lua Code Box)"))
	void SetText(FText InText);

	UFUNCTION(BlueprintPure, Category = "Widget", meta = (DisplayName = "GetCursorLine (Lua Code Box)"))
	int32 GetCursorLine() const;

	UFUNCTION(BlueprintPure, Category = "Widget", meta = (DisplayName = "GetCursorColumn (Lua Code Box)"))
	int32 GetCursorColumn() const;

	UFUNCTION(BlueprintCallable, Category = "Widget", meta = (DisplayName = "MoveCursorUp (Lua Code Box)"))
	void MoveCursorUp();

	UFUNCTION(BlueprintCallable, Category = "Widget", meta = (DisplayName = "MoveCursorDown (Lua Code Box)"))
	void MoveCursorDown();

	UFUNCTION(BlueprintCallable, Category = "Widget", meta = (DisplayName = "MoveCursorRight (Lua Code Box)"))
	void MoveCursorRight();

	UFUNCTION(BlueprintCallable, Category = "Widget", meta = (DisplayName = "MoveCursorLeft (Lua Code Box)"))
	void MoveCursorLeft();

	UFUNCTION(BlueprintCallable, Category = "Widget", meta = (DisplayName = "CursorGoTo (Lua Code Box)"))
	void CursorGoTo(int32 Line, int32 Column);

	UFUNCTION(BlueprintPure, Category = "Widget", meta = (DisplayName = "GetSelectedText (Lua Code Box)"))
	FText GetSelectedText() const;

	virtual FReply OnKeyChar(const FGeometry& InGeometry, const FCharacterEvent& InCharacterEvent);

	virtual FReply OnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent);

	virtual void OnCursorMoved(const FTextLocation& Location);

	virtual void SynchronizeProperties() override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	
	TSharedPtr<SMultiLineEditableTextBox> EditableTextBoxPtr;

	FTextLocation CursorLocation;

#if ENGINE_MAJOR_VERSION >=5 && ENGINE_MINOR_VERSION >= 1
	FEditableTextBoxStyle EditableTextBoxStyle;
#endif
};
