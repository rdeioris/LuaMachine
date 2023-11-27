// Copyright 2018-2023 - Roberto De Ioris


#include "LuaCommandExecutor.h"
#include "LuaBlueprintFunctionLibrary.h"

FLuaCommandExecutor::FLuaCommandExecutor()
{
	LuaState = nullptr;
}

FLuaCommandExecutor::~FLuaCommandExecutor()
{
}

FName FLuaCommandExecutor::GetName() const
{
	return FName(*FString::Printf(TEXT("LuaMachine:%s"), *LuaState->GetName()));
}

FText FLuaCommandExecutor::GetDisplayName() const
{
	return FText::FromName(GetName());
}

FText FLuaCommandExecutor::GetDescription() const
{
	return GetDisplayName();
}

FText FLuaCommandExecutor::GetHintText() const
{
	return FText::GetEmpty();
}

void FLuaCommandExecutor::GetAutoCompleteSuggestions(const TCHAR* Input, TArray<FString>& Out)
{
}

void FLuaCommandExecutor::GetExecHistory(TArray<FString>& Out)
{
	IConsoleManager::Get().GetConsoleHistory(*(GetName().ToString()), Out);
}

bool FLuaCommandExecutor::Exec(const TCHAR* Input)
{
	IConsoleManager::Get().AddConsoleHistoryEntry(*(GetName().ToString()), Input);

	LuaState->RunString(Input, "");

	return true;
}

bool FLuaCommandExecutor::AllowHotKeyClose() const
{
	return false;
}

bool FLuaCommandExecutor::AllowMultiLine() const
{
	return true;
}

FInputChord FLuaCommandExecutor::GetHotKey() const
{
	return FInputChord();
}


