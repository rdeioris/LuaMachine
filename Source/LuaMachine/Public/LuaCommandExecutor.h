// Copyright 2018-2023 - Roberto De Ioris

#pragma once

#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"
#include "Framework/Commands/InputChord.h"
#include "Runtime/Launch/Resources/Version.h"

class ULuaState;

/**
 * 
 */
class LUAMACHINE_API FLuaCommandExecutor : public IConsoleCommandExecutor
{
public:
	FLuaCommandExecutor();
	~FLuaCommandExecutor();

	FName GetName() const override;
	FText GetDisplayName() const override;
	FText GetDescription() const override;
	FText GetHintText() const override;
	void GetAutoCompleteSuggestions(const TCHAR* Input, TArray<FString>& Out) override;
	void GetExecHistory(TArray<FString>& Out) override;
	bool Exec(const TCHAR* Input) override;
	bool AllowHotKeyClose() const override;
	bool AllowMultiLine() const override;
	FInputChord GetHotKey() const override;

#if ENGINE_MAJOR_VERSION > 4 && ENGINE_MINOR_VERSION > 0
	FInputChord GetIterateExecutorHotKey() const
	{
		return {};
	}
#endif

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 5
	void GetSuggestedCompletions(const TCHAR* Input, TArray<FConsoleSuggestion>& Out) override {}
#endif

	ULuaState* LuaState;
};
