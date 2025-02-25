// Copyright 2018-2023 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "Editor/PropertyEditor/Public/IDetailCustomization.h"

/**
 * 
 */

class FLuaCodeCustomization : public IDetailCustomization
{
public:
	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FLuaCodeCustomization());
	}
};

