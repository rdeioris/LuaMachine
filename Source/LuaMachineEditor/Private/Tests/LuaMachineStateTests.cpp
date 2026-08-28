// Copyright 2025 - Roberto De Ioris

#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/LuaUnitTestState.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineStateTest_Integer, "LuaMachine.UnitTests.State.Integer", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineStateTest_Integer::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	FLuaValue LuaValue = UnitTestState->RunString("return 1 + 1", "");

	TestTrue(TEXT("LuaValue.Integer == 2"), LuaValue.Integer == 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineStateTest_String, "LuaMachine.UnitTests.State.String", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineStateTest_String::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	FLuaValue LuaValue = UnitTestState->RunString("return \"lua\"", "");

	TestTrue(TEXT("LuaValue.String == \"lua\""), LuaValue.String == "lua");

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineStateTest_Call, "LuaMachine.UnitTests.State.Call", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineStateTest_Call::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	UnitTestState->RunString("testtable = { testfunction = function() return \"lua\" end }", "");

	FLuaValue LuaTestFunction = UnitTestState->GetLuaValueFromGlobalName("testtable.testfunction");

	TestTrue(TEXT("LuaValue.String == \"lua\""), UnitTestState->LuaValueCall(LuaTestFunction, {}).String == "lua");

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineStateTest_UObject, "LuaMachine.UnitTests.State.UObject", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineStateTest_UObject::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	FLuaValue ComparisonFunction = UnitTestState->RunString("return function(a, b) return a == b; end", "");

	TestTrue(TEXT("LuaValue.Bool == true"), UnitTestState->LuaValueCall(ComparisonFunction, { FLuaValue(TestWorld), FLuaValue(TestWorld) }).Bool);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineStateTest_Lambda, "LuaMachine.UnitTests.State.Lambda", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineStateTest_Lambda::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	FLuaValue ReturnValue = UnitTestState->RunString("return lambda001()", "");

	TestEqual(TEXT("LuaValue.String == \"Hello Test\""), ReturnValue.ToString(), "Hello Test");

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineStateTest_LambdaReturningLambda, "LuaMachine.UnitTests.State.LambdaReturningLambda", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineStateTest_LambdaReturningLambda::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	FLuaValue ReturnValue = UnitTestState->RunString("return lambda002()()", "");

	TestEqual(TEXT("LuaValue.String == \"Hello Test\""), ReturnValue.ToString(), "Hello Test");

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineStateTest_UFunction, "LuaMachine.UnitTests.State.UFunction", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineStateTest_UFunction::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	FLuaValue ReturnValue = UnitTestState->RunString("return dummy()", "");

	TestEqual(TEXT("LuaValue.String == \"Hello Test\""), ReturnValue.ToString(), "Hello Test");

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineStateTest_LambdaError, "LuaMachine.UnitTests.State.LambdaError", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineStateTest_LambdaError::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	UnitTestState->bLogError = false;

	FLuaValue ReturnValue = UnitTestState->RunString("return lambda003()", "");

	TestTrue(TEXT("ReturnValue == nil"), ReturnValue.IsNil());
	TestTrue(TEXT("LuaState Error"), UnitTestState->LastError.Contains("!!!ERROR!!!"));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineStateTest_FunctionsArrayCall, "LuaMachine.UnitTests.State.FunctionsArrayCall", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineStateTest_FunctionsArrayCall::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	FLuaValue LuaFunctionsArray = UnitTestState->RunString(R"(
		return {
			function() return "lua" end,
			function() return 100 end, 
			function() return false end,
		}
	)", "");

	TestTrue(TEXT("LuaValue[1].String == \"lua\""), UnitTestState->LuaValueCall(LuaFunctionsArray.GetFieldByIndex(1), {}).String == "lua");
	TestTrue(TEXT("LuaValue[1].Integer == 100"), UnitTestState->LuaValueCall(LuaFunctionsArray.GetFieldByIndex(2), {}).Integer == 100);
	TestTrue(TEXT("LuaValue[1].Bool == false"), UnitTestState->LuaValueCall(LuaFunctionsArray.GetFieldByIndex(3), {}).Bool == false);

	return true;
}

// Regression test: FLuaValue::operator= was not calling Unref() on the existing value before
// overwriting LuaRef. When a C++ FLuaValue holding a Lua Table is reassigned, the old registry
// ref was silently abandoned and the table could never be collected by Lua's GC.
//
// We use a Lua weak table to track the returned tables. After releasing all C++ refs and
// forcing GC, any entry that is still alive (non-nil) has an orphaned strong ref keeping it alive.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineStateTest_UFunctionReturningTable_NoLuaRefLeak,
	"LuaMachine.UnitTests.State.UFunctionReturningTable.NoLuaRefLeak",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineStateTest_UFunctionReturningTable_NoLuaRefLeak::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	// Weak table: values are held weakly, so a table stored here won't prevent GC.
	// If an entry survives GC it means a strong ref (leaked LuaRef) is keeping it alive.
	FLuaValue WeakTable = UnitTestState->RunString(
		"return setmetatable({}, {__mode = 'v'})", "");

	FLuaValue ReturnTableFunc = UnitTestState->GetLuaValueFromGlobalName("return_table");

	// Reuse the same C++ variable -- each iteration calls FLuaValue::operator= on an
	// FLuaValue that already holds a Table ref. Without the Unref() fix the old ref leaks.
	FLuaValue Result;
	const int32 NumCalls = 10;
	for (int32 i = 1; i <= NumCalls; i++)
	{
		Result = UnitTestState->LuaValueCall(ReturnTableFunc, {});
		WeakTable.SetFieldByIndex(i, Result);  // store weakly
	}
	Result.Unref();  // release the last strong ref

	// After a full GC cycle, every table must have been collected.
	// Any surviving entry means its LuaRef was never released (operator= bug).
	UnitTestState->GC(LUA_GCCOLLECT);

	for (int32 i = 1; i <= NumCalls; i++)
	{
		TestTrue(
			FString::Printf(TEXT("Weak table entry [%d] is nil after GC (no leaked LuaRef)"), i),
			WeakTable.GetFieldByIndex(i).IsNil());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineStateTest_RunStringMultiStackBalance, "LuaMachine.UnitTests.State.RunStringMultiStackBalance", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineStateTest_RunStringMultiStackBalance::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	const int32 StackTop = UnitTestState->GetTop();

	// a chunk returning nothing pushes nothing: popping unconditionally here
	// would eat a slot belonging to the caller
	TArray<FLuaValue> NoResults = UnitTestState->RunStringMulti("local unused = 1", "");
	TestTrue(TEXT("no return values"), NoResults.Num() == 0);
	TestTrue(TEXT("stack balanced after a chunk returning nothing"), UnitTestState->GetTop() == StackTop);

	TArray<FLuaValue> Results = UnitTestState->RunStringMulti("return 1, \"two\", true", "");
	TestTrue(TEXT("three return values in source order"), Results.Num() == 3 &&
		Results[0].ToInteger() == 1 && Results[1].String == "two" && Results[2].Bool);
	TestTrue(TEXT("stack balanced after a chunk returning three values"), UnitTestState->GetTop() == StackTop);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineStateTest_LuaValueCallMultiStackBalance, "LuaMachine.UnitTests.State.LuaValueCallMultiStackBalance", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineStateTest_LuaValueCallMultiStackBalance::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	UnitTestState->RunString("function noret() end\nfunction multiret() return \"a\", \"b\" end", "");

	const int32 StackTop = UnitTestState->GetTop();

	FLuaValue NoRet = UnitTestState->GetLuaValueFromGlobalName("noret");
	TArray<FLuaValue> NoResults = UnitTestState->LuaValueCallMulti(NoRet, {});
	TestTrue(TEXT("no return values"), NoResults.Num() == 0);
	TestTrue(TEXT("stack balanced after a call returning nothing"), UnitTestState->GetTop() == StackTop);

	FLuaValue MultiRet = UnitTestState->GetLuaValueFromGlobalName("multiret");
	TArray<FLuaValue> Results = UnitTestState->LuaValueCallMulti(MultiRet, {});
	TestTrue(TEXT("two return values in order"), Results.Num() == 2 &&
		Results[0].String == "a" && Results[1].String == "b");
	TestTrue(TEXT("stack balanced after a call returning two values"), UnitTestState->GetTop() == StackTop);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineStateTest_LambdaRoundTrip, "LuaMachine.UnitTests.State.LambdaRoundTrip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineStateTest_LambdaRoundTrip::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	// a lambda pushed into lua must come back out as a Lambda, not as nil
	FLuaValue Back = UnitTestState->GetLuaValueFromGlobalName("lambda001");
	TestTrue(TEXT("lambda read back from lua keeps its type"), Back.Type == ELuaValueType::Lambda);

	FLuaValue Result = UnitTestState->LuaValueCall(Back, {});
	TestTrue(TEXT("round-tripped lambda is still callable"), Result.String == "Hello Test");

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineStateTest_LambdaInCoroutine, "LuaMachine.UnitTests.State.LambdaInCoroutine", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineStateTest_LambdaInCoroutine::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	// inside a coroutine the calling lua_State is not the main one, so the
	// lambda's return value has to be pushed onto the coroutine's stack
	FLuaValue LuaValue = UnitTestState->RunString(
		"local co = coroutine.create(function() return lambda001() end)\n"
		"local ok, value = coroutine.resume(co)\n"
		"return value", "");

	TestTrue(TEXT("lambda called from a coroutine returns on the right stack"), LuaValue.String == "Hello Test");

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineStateTest_LambdaGC, "LuaMachine.UnitTests.State.LambdaNoLeakOnGC", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineStateTest_LambdaGC::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	// the sentinel is captured by the lambda, so its reference count tells us
	// whether the TFunction behind the lua userdata was ever destroyed
	TSharedPtr<int32> Sentinel = MakeShared<int32>(1234);
	TestTrue(TEXT("sentinel starts with a single owner"), Sentinel.GetSharedReferenceCount() == 1);

	{
		FLuaValue Lambda = FLuaValue::NewLambda([Sentinel](TArray<FLuaValue> Args) { return FLuaValue(*Sentinel); });
		UnitTestState->SetLuaValueFromGlobalName("gc_lambda", Lambda);
	}

	TestTrue(TEXT("lua owns the captured sentinel"), Sentinel.GetSharedReferenceCount() > 1);

	// drop the only reference from lua and let the collector run the finalizer
	UnitTestState->SetLuaValueFromGlobalName("gc_lambda", FLuaValue());
	UnitTestState->GC(LUA_GCCOLLECT);
	UnitTestState->GC(LUA_GCCOLLECT);

	TestTrue(TEXT("sentinel released once the lambda userdata is collected"), Sentinel.GetSharedReferenceCount() == 1);

	return true;
}

// package.preload used to consult only the content root: RunFile() reports success
// for a file that does not exist, so the root always "matched" and the entries of
// AppendProjectContentDirSubDir were never reached.
//
// Reaching that code needs a key that is in RequireTable when the state is built (so
// the preload entry gets installed) but gone by the time require runs -- otherwise
// the lookup resolves to the LuaCode asset and never touches the file search.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineStateTest_PreloadAdditionalPaths, "LuaMachine.UnitTests.State.PreloadAdditionalPaths", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineStateTest_PreloadAdditionalPaths::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	// lay out <root>/first/ (empty) and <root>/second/packageundertest.lua
	const FString ScriptRoot = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("LuaMachineTests"), TEXT("PreloadAdditionalPaths"));
	IFileManager::Get().DeleteDirectory(*ScriptRoot, false, true);
	IFileManager::Get().MakeDirectory(*FPaths::Combine(ScriptRoot, TEXT("first")), true);
	IFileManager::Get().MakeDirectory(*FPaths::Combine(ScriptRoot, TEXT("second")), true);

	const FString PackageFilename = FPaths::Combine(ScriptRoot, TEXT("second"), TEXT("packageundertest.lua"));
	if (!TestTrue(TEXT("fixture written"), FFileHelper::SaveStringToFile(TEXT("return \"found in second\""), *PackageFilename)))
	{
		return false;
	}

	ULuaUnitTestState* UnitTestState = NewObject<ULuaUnitTestState>(GetTransientPackage());
	UnitTestState->ScriptContentDirectory = ScriptRoot;
	UnitTestState->AppendProjectContentDirSubDir = { TEXT("first"), TEXT("second") };
	// installs package.preload["packageundertest"]
	UnitTestState->RequireTable.Add(TEXT("packageundertest"), nullptr);
	UnitTestState = Cast<ULuaUnitTestState>(UnitTestState->GetLuaState(TestWorld));

	if (!TestNotNull(TEXT("lua state created"), UnitTestState))
	{
		return false;
	}

	// drop the asset mapping so the preload handler falls through to the file search
	UnitTestState->RequireTable.Empty();

	FLuaValue LuaValue = UnitTestState->RunString("return require(\"packageundertest\")", "");

	// the root and "first" hold nothing, so this only resolves if every candidate is tried
	TestEqual(TEXT("package resolved from the second additional path"), LuaValue.ToString(), TEXT("found in second"));

	IFileManager::Get().DeleteDirectory(*ScriptRoot, false, true);

	return true;
}

#endif