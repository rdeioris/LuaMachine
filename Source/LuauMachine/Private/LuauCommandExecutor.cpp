// Copyright 2018-2023 - Roberto De Ioris
// Portions Copyright 2025 - Fusion Point Studios, Inc.


#include "LuauCommandExecutor.h"
#include "LuauBlueprintFunctionLibrary.h"

FLuauCommandExecutor::FLuauCommandExecutor()
{
	LuauState = nullptr;
}

FLuauCommandExecutor::~FLuauCommandExecutor()
{
}

FName FLuauCommandExecutor::GetName() const
{
	return FName(*FString::Printf(TEXT("LuauMachine:%s"), *LuauState->GetName()));
}

FText FLuauCommandExecutor::GetDisplayName() const
{
	return FText::FromName(GetName());
}

FText FLuauCommandExecutor::GetDescription() const
{
	return GetDisplayName();
}

FText FLuauCommandExecutor::GetHintText() const
{
	return FText::GetEmpty();
}

void FLuauCommandExecutor::GetAutoCompleteSuggestions(const TCHAR* Input, TArray<FString>& Out)
{
}

void FLuauCommandExecutor::GetExecHistory(TArray<FString>& Out)
{
	IConsoleManager::Get().GetConsoleHistory(*(GetName().ToString()), Out);
}

bool FLuauCommandExecutor::Exec(const TCHAR* Input)
{
	IConsoleManager::Get().AddConsoleHistoryEntry(*(GetName().ToString()), Input);

	LuauState->RunString(Input, "");

	return true;
}

bool FLuauCommandExecutor::AllowHotKeyClose() const
{
	return false;
}

bool FLuauCommandExecutor::AllowMultiLine() const
{
	return true;
}

FInputChord FLuauCommandExecutor::GetHotKey() const
{
	return FInputChord();
}


