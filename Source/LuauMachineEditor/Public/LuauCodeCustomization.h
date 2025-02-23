// Copyright 2018-2023 - Roberto De Ioris
// Portions Copyright 2025 - Fusion Point Studios, Inc.

#pragma once

#include "CoreMinimal.h"
#include "Editor/PropertyEditor/Public/IDetailCustomization.h"

/**
 * 
 */

class FLuauCodeCustomization : public IDetailCustomization
{
public:
	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FLuauCodeCustomization());
	}
};

