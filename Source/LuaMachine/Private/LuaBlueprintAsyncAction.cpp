// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaBlueprintAsyncAction.h"
#include "LuaState.h"
#include "Runtime/Core/Public/Async/AsyncWork.h"
#include "Runtime/Core/Public/Async/TaskGraphInterfaces.h"
#include "Runtime/Core/Public/Misc/Paths.h"
#include "Runtime/Core/Public/Misc/FileHelper.h"

class FLuaAsyncTask : public FNonAbandonableTask
{
	friend class FAutoDeleteAsyncTask<FLuaAsyncTask>;
public:
	FLuaAsyncTask(FString CodeString, FString CodePath, TArray<FLuaValue> Args, bool bLuaOpenLibs, FLuaBlueprintAsyncResult OnLuaAsyncResult) :
		CodeString(CodeString), CodePath(CodePath), Args(Args), bLuaOpenLibs(bLuaOpenLibs), OnLuaAsyncResult(OnLuaAsyncResult)
	{

	}

protected:
	void DoWork()
	{
		FString ErrorString;
		bool bValid = true;
		if (CodeString == "@")
		{
			FString AbsoluteFilename = FPaths::Combine(FPaths::ProjectContentDir(), CodePath);

			if (!FPaths::FileExists(AbsoluteFilename))
			{
				ErrorString = FString::Printf(TEXT("Unable to open file %s"), *AbsoluteFilename);
				bValid = false;
			}
			else
			{
				TArray<uint8> RawCode;
				if (FFileHelper::LoadFileToArray(RawCode, *AbsoluteFilename))
				{
					ErrorString = FString::Printf(TEXT("Unable to read file %s"), *AbsoluteFilename);
					bValid = false;
				}
				else
				{
					CodePath = AbsoluteFilename;
					CodeString = BytesToString(RawCode.GetData(), RawCode.Num());
				}
			}
		}
		TArray<FLuaValue> Output;

		if (bValid)
			Output = ULuaState::StateLessSafeRun(CodeString, CodePath, bLuaOpenLibs, Args, ErrorString);

		FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, Output, ErrorString]()
		{
			OnLuaAsyncResult.Broadcast(Output, ErrorString);
		}, TStatId(), NULL, ENamedThreads::GameThread);
		FTaskGraphInterface::Get().WaitUntilTaskCompletes(Task);
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FLuaAsyncTask, STATGROUP_ThreadPoolAsyncTasks);
	}

	FString CodeString;
	FString CodePath;
	TArray<FLuaValue> Args;
	bool bLuaOpenLibs;
	FLuaBlueprintAsyncResult OnLuaAsyncResult;
};

ULuaBlueprintAsyncAction* ULuaBlueprintAsyncAction::LuaAsyncRunFromString(UObject* WorldContextObject, FString CodeString, TArray<FLuaValue> Args, bool bLuaOpenLibs)
{
	ULuaBlueprintAsyncAction* AsyncAction = NewObject<ULuaBlueprintAsyncAction>();
	AsyncAction->CodeString = CodeString;
	AsyncAction->CodePath = CodeString;
	AsyncAction->bLuaOpenLibs = bLuaOpenLibs;
	AsyncAction->Args = Args;
	return AsyncAction;
}

ULuaBlueprintAsyncAction* ULuaBlueprintAsyncAction::LuaAsyncRunFromCodeAsset(UObject* WorldContextObject, ULuaCode* CodeAsset, TArray<FLuaValue> Args, bool bLuaOpenLibs)
{
	ULuaBlueprintAsyncAction* AsyncAction = NewObject<ULuaBlueprintAsyncAction>();
	if (CodeAsset->bCooked && CodeAsset->bCookAsBytecode)
	{
#if PLATFORM_ANDROID
		// fix size_t of the bytecode
		if (CodeAsset->ByteCode.Num() >= 14)
			CodeAsset->ByteCode[13] = sizeof(size_t);
#endif
		AsyncAction->CodeString = BytesToString(CodeAsset->ByteCode.GetData(), CodeAsset->ByteCode.Num());
	}
	else
	{
		AsyncAction->CodeString = CodeAsset->Code.ToString();
	}
	AsyncAction->CodePath = CodeAsset->GetPathName();
	AsyncAction->bLuaOpenLibs = bLuaOpenLibs;
	AsyncAction->Args = Args;
	return AsyncAction;
}

ULuaBlueprintAsyncAction* ULuaBlueprintAsyncAction::LuaAsyncRunFromFile(UObject* WorldContextObject, FString Filename, TArray<FLuaValue> Args, bool bLuaOpenLibs)
{

	ULuaBlueprintAsyncAction* AsyncAction = NewObject<ULuaBlueprintAsyncAction>();
	AsyncAction->CodeString = "@";
	AsyncAction->CodePath = Filename;
	AsyncAction->bLuaOpenLibs = bLuaOpenLibs;
	AsyncAction->Args = Args;
	return AsyncAction;
}

void ULuaBlueprintAsyncAction::Activate()
{
	(new FAutoDeleteAsyncTask<FLuaAsyncTask>(CodeString, CodePath, Args, bLuaOpenLibs, OnLuaAsyncResult))->StartBackgroundTask();
}
