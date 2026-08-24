// Copyright 2025 - Roberto De Ioris

#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/LuaUnitTestState.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineCoroutineTest_Simple, "LuaMachine.UnitTests.Coroutine.Simple", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineCoroutineTest_Simple::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	FLuaValue LuaCoroutine = UnitTestState->RunString("return coroutine.create(function() coroutine.yield(100); coroutine.yield(200); coroutine.yield(300); end)", "");

	TArray<FLuaValue> Results = UnitTestState->LuaValueResume(LuaCoroutine, {});

	TestTrue(TEXT("LuaCoroutine is active"), Results.Num() == 2 && Results[0].Type == ELuaValueType::Bool && Results[0].Bool);
	TestTrue(TEXT("LuaCoroutine yields 100"), Results[1].ToInteger() == 100);

	Results = UnitTestState->LuaValueResume(LuaCoroutine, {});

	TestTrue(TEXT("LuaCoroutine is active"), Results.Num() == 2 && Results[0].Type == ELuaValueType::Bool && Results[0].Bool);
	TestTrue(TEXT("LuaCoroutine yields 200"), Results[1].ToInteger() == 200);

	Results = UnitTestState->LuaValueResume(LuaCoroutine, {});

	TestTrue(TEXT("LuaCoroutine is active"), Results.Num() == 2 && Results[0].Type == ELuaValueType::Bool && Results[0].Bool);
	TestTrue(TEXT("LuaCoroutine yields 300"), Results[1].ToInteger() == 300);

	Results = UnitTestState->LuaValueResume(LuaCoroutine, {});

	TestTrue(TEXT("LuaCoroutine is active (last)"), Results.Num() == 1 && Results[0].Type == ELuaValueType::Bool && Results[0].Bool);

	Results = UnitTestState->LuaValueResume(LuaCoroutine, {});

	TestTrue(TEXT("LuaCoroutine is not active"), Results.Num() == 2 && Results[0].Type == ELuaValueType::Bool &&
		!Results[0].Bool && Results[1].ToString() == "Lua error: cannot resume dead coroutine");

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineCoroutineTest_Return, "LuaMachine.UnitTests.Coroutine.Return", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineCoroutineTest_Return::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	FLuaValue LuaCoroutine = UnitTestState->RunString("return coroutine.create(function() coroutine.yield(100); coroutine.yield(200); coroutine.yield(300); return 1000 end)", "");

	TArray<FLuaValue> Results = UnitTestState->LuaValueResume(LuaCoroutine, {});

	TestTrue(TEXT("LuaCoroutine is active"), Results.Num() == 2 && Results[0].Type == ELuaValueType::Bool && Results[0].Bool);
	TestTrue(TEXT("LuaCoroutine yields 100"), Results[1].ToInteger() == 100);

	Results = UnitTestState->LuaValueResume(LuaCoroutine, {});

	TestTrue(TEXT("LuaCoroutine is active"), Results.Num() == 2 && Results[0].Type == ELuaValueType::Bool && Results[0].Bool);
	TestTrue(TEXT("LuaCoroutine yields 200"), Results[1].ToInteger() == 200);

	Results = UnitTestState->LuaValueResume(LuaCoroutine, {});

	TestTrue(TEXT("LuaCoroutine is active"), Results.Num() == 2 && Results[0].Type == ELuaValueType::Bool && Results[0].Bool);
	TestTrue(TEXT("LuaCoroutine yields 300"), Results[1].ToInteger() == 300);

	Results = UnitTestState->LuaValueResume(LuaCoroutine, {});

	TestTrue(TEXT("LuaCoroutine is active"), Results.Num() == 2 && Results[0].Type == ELuaValueType::Bool && Results[0].Bool);
	TestTrue(TEXT("LuaCoroutine returns 1000"), Results[1].ToInteger() == 1000);

	Results = UnitTestState->LuaValueResume(LuaCoroutine, {});

	TestTrue(TEXT("LuaCoroutine is not active"), Results.Num() == 2 && Results[0].Type == ELuaValueType::Bool &&
		!Results[0].Bool && Results[1].ToString() == "Lua error: cannot resume dead coroutine");

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineCoroutineTest_Args, "LuaMachine.UnitTests.Coroutine.Args", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineCoroutineTest_Args::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	FLuaValue LuaCoroutine = UnitTestState->RunString("return coroutine.create(function(x) y = coroutine.yield(100 * x); z = coroutine.yield(200 * x * y); coroutine.yield(300 * x * y * z); end)", "");

	TArray<FLuaValue> Results = UnitTestState->LuaValueResume(LuaCoroutine, { 2 });

	TestTrue(TEXT("LuaCoroutine is active"), Results.Num() == 2 && Results[0].Type == ELuaValueType::Bool && Results[0].Bool);
	TestTrue(TEXT("LuaCoroutine yields 200"), Results[1].ToInteger() == 200);

	Results = UnitTestState->LuaValueResume(LuaCoroutine, { 5 });

	TestTrue(TEXT("LuaCoroutine is active"), Results.Num() == 2 && Results[0].Type == ELuaValueType::Bool && Results[0].Bool);
	TestTrue(TEXT("LuaCoroutine yields 2000"), Results[1].ToInteger() == 2000);

	Results = UnitTestState->LuaValueResume(LuaCoroutine, { 10 });

	TestTrue(TEXT("LuaCoroutine is active"), Results.Num() == 2 && Results[0].Type == ELuaValueType::Bool && Results[0].Bool);
	TestTrue(TEXT("LuaCoroutine yields 30000"), Results[1].ToInteger() == 30000);

	Results = UnitTestState->LuaValueResume(LuaCoroutine, {});

	TestTrue(TEXT("LuaCoroutine is active (last)"), Results.Num() == 1 && Results[0].Type == ELuaValueType::Bool && Results[0].Bool);

	Results = UnitTestState->LuaValueResume(LuaCoroutine, {});

	TestTrue(TEXT("LuaCoroutine is not active"), Results.Num() == 2 && Results[0].Type == ELuaValueType::Bool &&
		!Results[0].Bool && Results[1].ToString() == "Lua error: cannot resume dead coroutine");

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineCoroutineTest_QuickReturn, "LuaMachine.UnitTests.Coroutine.QuickReturn", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineCoroutineTest_QuickReturn::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	FLuaValue LuaCoroutine = UnitTestState->RunString("return coroutine.create(function() return 10000 end)", "");

	TArray<FLuaValue> Results = UnitTestState->LuaValueResume(LuaCoroutine, {});

	TestTrue(TEXT("LuaCoroutine is active"), Results.Num() == 2 && Results[0].Type == ELuaValueType::Bool && Results[0].Bool);
	TestTrue(TEXT("LuaCoroutine returns 10000"), Results[1].ToInteger() == 10000);

	Results = UnitTestState->LuaValueResume(LuaCoroutine, {});

	TestTrue(TEXT("LuaCoroutine is not active"), Results.Num() == 2 && Results[0].Type == ELuaValueType::Bool &&
		!Results[0].Bool && Results[1].ToString() == "Lua error: cannot resume dead coroutine");

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineCoroutineTest_QuickReturnMulti, "LuaMachine.UnitTests.Coroutine.QuickReturnMulti", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineCoroutineTest_QuickReturnMulti::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	FLuaValue LuaCoroutine = UnitTestState->RunString("return coroutine.create(function() return \"test\", 999, true end)", "");

	TArray<FLuaValue> Results = UnitTestState->LuaValueResume(LuaCoroutine, {});

	TestTrue(TEXT("LuaCoroutine is active"), Results.Num() == 4 && Results[0].Type == ELuaValueType::Bool && Results[0].Bool);
	TestTrue(TEXT("LuaCoroutine returns \"test\", 999, true"), Results[1].String == "test" && Results[2].ToInteger() == 999 && Results[3].Bool);

	Results = UnitTestState->LuaValueResume(LuaCoroutine, {});

	TestTrue(TEXT("LuaCoroutine is not active"), Results.Num() == 2 && Results[0].Type == ELuaValueType::Bool &&
		!Results[0].Bool && Results[1].ToString() == "Lua error: cannot resume dead coroutine");

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineCoroutineTest_YieldMulti, "LuaMachine.UnitTests.Coroutine.YieldMulti", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineCoroutineTest_YieldMulti::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	FLuaValue LuaCoroutine = UnitTestState->RunString("return coroutine.create(function() coroutine.yield(\"test\", 999, true) end)", "");

	TArray<FLuaValue> Results = UnitTestState->LuaValueResume(LuaCoroutine, {});

	TestTrue(TEXT("LuaCoroutine is active"), Results.Num() == 4 && Results[0].Type == ELuaValueType::Bool && Results[0].Bool);
	TestTrue(TEXT("LuaCoroutine yields \"test\", 999, true"), Results[1].String == "test" && Results[2].ToInteger() == 999 && Results[3].Bool);

	Results = UnitTestState->LuaValueResume(LuaCoroutine, {});

	TestTrue(TEXT("LuaCoroutine is active (last)"), Results.Num() == 1 && Results[0].Type == ELuaValueType::Bool && Results[0].Bool);

	Results = UnitTestState->LuaValueResume(LuaCoroutine, {});

	TestTrue(TEXT("LuaCoroutine is not active"), Results.Num() == 2 && Results[0].Type == ELuaValueType::Bool &&
		!Results[0].Bool && Results[1].ToString() == "Lua error: cannot resume dead coroutine");

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineCoroutineTest_WrongArgs, "LuaMachine.UnitTests.Coroutine.WrongArgs", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineCoroutineTest_WrongArgs::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	FLuaValue LuaCoroutine = UnitTestState->RunString("return coroutine.create(function() x = coroutine.yield(\"test\", 999, true); return x * 2 end)", "");

	TArray<FLuaValue> Results = UnitTestState->LuaValueResume(LuaCoroutine, {});

	TestTrue(TEXT("LuaCoroutine is active"), Results.Num() == 4 && Results[0].Type == ELuaValueType::Bool && Results[0].Bool);
	TestTrue(TEXT("LuaCoroutine yields \"test\", 999, true"), Results[1].String == "test" && Results[2].ToInteger() == 999 && Results[3].Bool);

	Results = UnitTestState->LuaValueResume(LuaCoroutine, {});

	TestTrue(TEXT("LuaCoroutine error (missing arg)"), Results.Num() == 2 && Results[0].Type == ELuaValueType::Bool && !Results[0].Bool && Results[1].ToString().Contains("nil"));

	return true;
}

#endif