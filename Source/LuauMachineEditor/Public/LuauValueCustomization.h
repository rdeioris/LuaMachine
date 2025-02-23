// Copyright 2018-2023 - Roberto De Ioris
// Portions Copyright 2025 - Fusion Point Studios, Inc.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Editor/PropertyEditor/Public/IPropertyTypeCustomization.h"
#include "Runtime/SlateCore/Public/Layout/Visibility.h"
#include "LuauMachine/Public/LuauValue.h"

class FLuauValueCustomization : public IPropertyTypeCustomization
{
public:
	// IPropertyTypeCustomization interface
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& Builder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShareable(new FLuauValueCustomization());
	}

	EVisibility IsPropertyVisible(TSharedRef<IPropertyHandle> PropertyHandle, ELuauValueType WantedValueType);

	void LuauFunctionChanged(TSharedPtr<FString> Value, ESelectInfo::Type SelectionType, TSharedRef<IPropertyHandle> PropertyHandle);
protected:
	TArray<TSharedPtr<FString>> ValidLuauFunctions;
};

