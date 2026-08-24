// Copyright 2025 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "LuaMachineK2Node_MakeLuaCodeString.generated.h"

/**
 * 
 */
UCLASS()
class LUAMACHINEBLUEPRINTNODES_API ULuaMachineK2Node_MakeLuaCodeString : public UK2Node
{
	GENERATED_BODY()

public:

	//K2Node implementation
	virtual FText GetMenuCategory() const override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	//K2Node implementation

	//UEdGraphNode implementation
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual TSharedPtr<SGraphNode> CreateVisualWidget() override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual bool IsNodePure() const override { return true; }
	//UEdGraphNode implementation

	const FString& GetLuaCodeString() const;

	void UpdateLuaCodeString(const FText& NewText);
	FReply OnKeyChar(const FGeometry& InGeometry, const FCharacterEvent& InCharacterEvent);

	TSharedPtr<class ITextLayoutMarshaller> GetSyntaxHighlighter() const;

protected:
	UPROPERTY()
	FString LuaCodeString;
	
	TSharedPtr<class SMultiLineEditableTextBox> LuaCodeStringEditorWidget;
};
