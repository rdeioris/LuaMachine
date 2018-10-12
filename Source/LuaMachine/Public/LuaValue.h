// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Editor/PropertyEditor/Public/IPropertyTypeCustomization.h"
#include "Runtime/SlateCore/Public/Layout/Visibility.h"
#include "LuaValue.generated.h"


/**
 *
 */

UENUM(BlueprintType)
enum class ELuaValueType : uint8
{
	Nil,
	Bool,
	Integer,
	Number,
	String,
	UserData,
	Function,
	Thread,
	Table,
	Object,
};

USTRUCT(BlueprintType)
struct LUAMACHINE_API FLuaValue
{
	GENERATED_BODY()

	FLuaValue()
	{
		Type = ELuaValueType::Nil;
	}

	FLuaValue(FString InString)
	{
		Type = ELuaValueType::String;
		String = InString;
	}

	FLuaValue(int32 Value)
	{
		Type = ELuaValueType::Integer;
		Integer = Value;
	}

	FLuaValue(UObject* Object)
	{
		Type = ELuaValueType::Object;
		ObjectPath = FSoftObjectPath(Object);
	}

	static FLuaValue Function(FName FunctionName)
	{
		FLuaValue LuaValue;
		LuaValue.Type = ELuaValueType::Function;
		LuaValue.FunctionName = FunctionName;
		return LuaValue;
	}

	FString ToString();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ELuaValueType Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool Bool;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Integer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Number;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString String;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSoftObjectPath ObjectPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName FunctionName;
};

class FLuaValueCustomization : public IPropertyTypeCustomization
{
public:
	// IPropertyTypeCustomization interface
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& Builder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShareable(new FLuaValueCustomization());
	}

	EVisibility IsPropertyVisible(TSharedRef<IPropertyHandle> PropertyHandle, ELuaValueType WantedValueType);

	void LuaFunctionChanged(TSharedPtr<FString> Value, ESelectInfo::Type SelectionType, TSharedRef<IPropertyHandle> PropertyHandle);
protected:
	TArray<TSharedPtr<FString>> ValidLuaFunctions;
};

