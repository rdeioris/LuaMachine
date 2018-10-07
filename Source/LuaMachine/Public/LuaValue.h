// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
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
	Table,
	UserData,
	Function,
	Thread,
	Error,
};

USTRUCT(BlueprintType)
struct LUAMACHINE_API FLuaValue
{
	GENERATED_BODY()

	FLuaValue()
	{
		Type = ELuaValueType::Nil;
	}

	FLuaValue(ELuaValueType InType, FString InString)
	{
		Type = InType;
		String = InString;
	}

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
};
