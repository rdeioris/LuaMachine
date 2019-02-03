// Copyright 2019 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "Components/TextWidgetTypes.h"
#include "Components/MultiLineEditableTextBox.h"
#include "LuaMultiLineEditableTextBox.generated.h"

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

	UFUNCTION(BlueprintCallable, Category = "Widget", meta = (DisplayName = "GetText (Lua Code Box)"))
	FText GetText() const;

	UFUNCTION(BlueprintCallable, Category = "Widget", meta = (DisplayName = "SetText (Lua Code Box)"))
	void SetText(FText InText);

	virtual FReply OnKeyChar(const FGeometry& InGeometry, const FCharacterEvent& InCharacterEvent);

	virtual FReply OnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent);

	virtual void SynchronizeProperties() override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	
	TSharedPtr<SMultiLineEditableTextBox> EditableTextBoxPtr;
};
