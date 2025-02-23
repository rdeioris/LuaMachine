// Copyright 2018-2023 - Roberto De Ioris
// Portions Copyright 2025 - Fusion Point Studios, Inc.

#pragma once

#include "CoreMinimal.h"
#include "Components/TextWidgetTypes.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Runtime/Launch/Resources/Version.h"
#include "LuauMultiLineEditableTextBox.generated.h"

USTRUCT(BlueprintType)
struct LUAUMACHINE_API FLuauCustomHighlighter
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Luau")
	FLinearColor Color;

	UPROPERTY(EditAnywhere, Category = "Luau")
	TArray<FString> Tokens;

	FLuauCustomHighlighter();
};

/**
 * 
 */
UCLASS(meta=(DisplayName="Luau Code Box"))
class LUAUMACHINE_API ULuauMultiLineEditableTextBox : public UTextLayoutWidget
{
	GENERATED_BODY()

public:
	ULuauMultiLineEditableTextBox();

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Luau Code", meta = (DisplayName = "Tab Size"))
	int32 TabSize;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	bool bIsReadonly;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Luau Code")
	bool bHandleTab;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Luau Code")
	bool bHandleArrows;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style")
	TArray<FLuauCustomHighlighter> CustomTokensMapping;

	UFUNCTION(BlueprintPure, Category = "Widget", meta = (DisplayName = "GetText (Luau Code Box)"))
	FText GetText() const;

	UFUNCTION(BlueprintCallable, Category = "Widget", meta = (DisplayName = "SetText (Luau Code Box)"))
	void SetText(FText InText);

	UFUNCTION(BlueprintPure, Category = "Widget", meta = (DisplayName = "GetCursorLine (Luau Code Box)"))
	int32 GetCursorLine() const;

	UFUNCTION(BlueprintPure, Category = "Widget", meta = (DisplayName = "GetCursorColumn (Luau Code Box)"))
	int32 GetCursorColumn() const;

	UFUNCTION(BlueprintCallable, Category = "Widget", meta = (DisplayName = "MoveCursorUp (Luau Code Box)"))
	void MoveCursorUp();

	UFUNCTION(BlueprintCallable, Category = "Widget", meta = (DisplayName = "MoveCursorDown (Luau Code Box)"))
	void MoveCursorDown();

	UFUNCTION(BlueprintCallable, Category = "Widget", meta = (DisplayName = "MoveCursorRight (Luau Code Box)"))
	void MoveCursorRight();

	UFUNCTION(BlueprintCallable, Category = "Widget", meta = (DisplayName = "MoveCursorLeft (Luau Code Box)"))
	void MoveCursorLeft();

	UFUNCTION(BlueprintCallable, Category = "Widget", meta = (DisplayName = "CursorGoTo (Luau Code Box)"))
	void CursorGoTo(int32 Line, int32 Column);

	UFUNCTION(BlueprintPure, Category = "Widget", meta = (DisplayName = "GetSelectedText (Luau Code Box)"))
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
