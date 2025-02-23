// Copyright 2018-2023 - Roberto De Ioris

#include "LuauValueCustomization.h"
#include "LuauMachine/Public/LuauValue.h"
#include "LuauMachine/Public/LuauComponent.h"
#include "GameFramework/Actor.h"
#include "Editor/PropertyEditor/Public/PropertyHandle.h"
#include "Editor/PropertyEditor/Public/DetailLayoutBuilder.h"
#include "Editor/PropertyEditor/Public/IDetailChildrenBuilder.h"
#include "Editor/PropertyEditor/Public/DetailWidgetRow.h"
#include "Runtime/Slate/Public/Widgets/Text/STextBlock.h"
#include "Runtime/Slate/Public/Widgets/Input/STextComboBox.h"
#include "Runtime/Engine/Classes/Engine/BlueprintGeneratedClass.h"
#include "Modules/ModuleManager.h"


void FLuauValueCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	TSharedPtr<IPropertyHandle> LuauValueTypeProperty = PropertyHandle->GetChildHandle(FName(TEXT("Type")));

	HeaderRow.NameContent()
		[
			PropertyHandle->CreatePropertyNameWidget()
		].ValueContent()[
			LuauValueTypeProperty->CreatePropertyValueWidget()
		];
}

EVisibility FLuauValueCustomization::IsPropertyVisible(TSharedRef<IPropertyHandle> PropertyHandle, ELuauValueType WantedValueType)
{
	TSharedPtr<IPropertyHandle> LuauValueTypeProperty = PropertyHandle->GetChildHandle(FName(TEXT("Type")));
	if (!LuauValueTypeProperty.IsValid())
		return EVisibility::Hidden;

	uint8 ValueType;
	LuauValueTypeProperty->GetValue(ValueType);

	if ((ELuauValueType)ValueType == WantedValueType)
		return EVisibility::Visible;

	return EVisibility::Hidden;
}

void FLuauValueCustomization::LuauFunctionChanged(TSharedPtr<FString> Value, ESelectInfo::Type SelectionType, TSharedRef<IPropertyHandle> PropertyHandle)
{
	TArray<UObject*> Objects;
	PropertyHandle->GetOuterObjects(Objects);

	if (Objects.Num() != 1)
		return;

	UClass* ObjectClass = Objects[0]->GetClass();

	ULuauComponent* LuauComponent = Cast<ULuauComponent>(Objects[0]);
	if (LuauComponent)
	{
		AActor* Actor = LuauComponent->GetOwner();
		if (Actor)
		{
			ObjectClass = Actor->GetClass();
		}
		else
		{
			UBlueprintGeneratedClass* BlueprintClass = Cast<UBlueprintGeneratedClass>(LuauComponent->GetOuter());
			if (BlueprintClass)
			{
				ObjectClass = BlueprintClass;
			}
		}
	}


	UFunction* FoundFunction = ObjectClass->FindFunctionByName(FName(*(*Value.Get())));
	if (FoundFunction)
	{
		PropertyHandle->SetValue(FoundFunction->GetName());
	}
}

void FLuauValueCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& Builder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	TArray<UObject*> Objects;
	PropertyHandle->GetOuterObjects(Objects);

	if (Objects.Num() != 1)
		return;

	TSharedPtr<IPropertyHandle> LuauValueBoolProperty = PropertyHandle->GetChildHandle(FName(TEXT("Bool")));
	IDetailPropertyRow& PropertyBoolRow = Builder.AddProperty(LuauValueBoolProperty.ToSharedRef());
	PropertyBoolRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateRaw(this, &FLuauValueCustomization::IsPropertyVisible, PropertyHandle, ELuauValueType::Bool)));

	TSharedPtr<IPropertyHandle> LuauValueStringProperty = PropertyHandle->GetChildHandle(FName(TEXT("String")));
	IDetailPropertyRow& PropertyStringRow = Builder.AddProperty(LuauValueStringProperty.ToSharedRef());
	PropertyStringRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateRaw(this, &FLuauValueCustomization::IsPropertyVisible, PropertyHandle, ELuauValueType::String)));

	TSharedPtr<IPropertyHandle> LuauValueIntegerProperty = PropertyHandle->GetChildHandle(FName(TEXT("Integer")));
	IDetailPropertyRow& PropertyIntegerRow = Builder.AddProperty(LuauValueIntegerProperty.ToSharedRef());
	PropertyIntegerRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateRaw(this, &FLuauValueCustomization::IsPropertyVisible, PropertyHandle, ELuauValueType::Integer)));

	TSharedPtr<IPropertyHandle> LuauValueNumberProperty = PropertyHandle->GetChildHandle(FName(TEXT("Number")));
	IDetailPropertyRow& PropertyNumberRow = Builder.AddProperty(LuauValueNumberProperty.ToSharedRef());
	PropertyNumberRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateRaw(this, &FLuauValueCustomization::IsPropertyVisible, PropertyHandle, ELuauValueType::Number)));

	TSharedPtr<IPropertyHandle> LuauValueObjectProperty = PropertyHandle->GetChildHandle(FName(TEXT("Object")));
	IDetailPropertyRow& PropertyObjectRow = Builder.AddProperty(LuauValueObjectProperty.ToSharedRef());
	PropertyObjectRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateRaw(this, &FLuauValueCustomization::IsPropertyVisible, PropertyHandle, ELuauValueType::UObject)));

	TSharedPtr<IPropertyHandle> LuauValueFunctionProperty = PropertyHandle->GetChildHandle(FName(TEXT("FunctionName")));

	FString CurrentFunctionName;
	LuauValueFunctionProperty->GetValue(CurrentFunctionName);

	TSharedPtr<FString> CurrentSelectedFunction;

	UClass* ObjectClass = Objects[0]->GetClass();

	bool bAllowsRawCall = false;

	ULuauComponent* LuauComponent = Cast<ULuauComponent>(Objects[0]);
	if (LuauComponent)
	{
		AActor* Actor = LuauComponent->GetOwner();
		if (Actor)
		{
			ObjectClass = Actor->GetClass();
		}
		else
		{
			UBlueprintGeneratedClass* BlueprintClass = Cast<UBlueprintGeneratedClass>(LuauComponent->GetOuter());
			if (BlueprintClass)
			{
				ObjectClass = BlueprintClass;
			}
		}
	}
	else if (ULuauState* LuauState = Cast<ULuauState>(Objects[0]))
	{
		bAllowsRawCall = LuauState->bRawLuauFunctionCall;
	}

	for (TFieldIterator<UFunction> Funcs(ObjectClass); Funcs; ++Funcs)
	{
		UFunction* Function = *Funcs;

		/*if (!Function->HasAnyFunctionFlags(EFunctionFlags::FUNC_Public))
			continue;*/

		bool bIsValid = true;

		if (!bAllowsRawCall)
		{
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
			for (TFieldIterator<FProperty> FArgs(Function); FArgs && FArgs->PropertyFlags & CPF_Parm; ++FArgs)
#else
			for (TFieldIterator<UProperty> FArgs(Function); FArgs && FArgs->PropertyFlags & CPF_Parm; ++FArgs)
#endif
			{
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
				FProperty* Prop = *FArgs;
				FStructProperty* LuauProp = CastField<FStructProperty>(Prop);
#else
				UProperty* Prop = *FArgs;
				UStructProperty* LuauProp = Cast<UStructProperty>(Prop);
#endif
				if (!LuauProp)
				{
					// check for array ?
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
					FArrayProperty* ArrayProp = CastField<FArrayProperty>(Prop);
#else
					UArrayProperty* ArrayProp = Cast<UArrayProperty>(Prop);
#endif
					if (ArrayProp)
					{
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
						LuauProp = CastField<FStructProperty>(ArrayProp->Inner);
#else
						LuauProp = Cast<UStructProperty>(ArrayProp->Inner);
#endif
						if (!LuauProp)
						{
							bIsValid = false;
							break;
						}
					}
					else {
						bIsValid = false;
						break;
					}
				}
				if (LuauProp->Struct != FLuauValue::StaticStruct())
				{
					bIsValid = false;
					break;
				}
			}
		}

		if (!bIsValid)
		{
			continue;
		}

		TSharedPtr<FString> FunctionNameSP = MakeShareable(new FString(Function->GetName()));

		if (CurrentFunctionName == Function->GetName())
		{
			CurrentSelectedFunction = FunctionNameSP;
		}

		ValidLuauFunctions.Add(FunctionNameSP);
	}


	Builder.AddCustomRow(FText::FromString(TEXT("Function"))).ValueContent()[
		SNew(STextComboBox).OptionsSource(&ValidLuauFunctions).OnSelectionChanged_Raw(this, &FLuauValueCustomization::LuauFunctionChanged, LuauValueFunctionProperty.ToSharedRef()).InitiallySelectedItem(CurrentSelectedFunction)
	].Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateRaw(this, &FLuauValueCustomization::IsPropertyVisible, PropertyHandle, ELuauValueType::UFunction)));
}