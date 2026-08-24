// Copyright 2025 - Roberto De Ioris

#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/LuaUnitTestState.h"
#include "LuaBlueprintFunctionLibrary.h"
#include "Misc/AutomationTest.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineUnicodeTest_Simple, "LuaMachine.UnitTests.Unicode.Simple", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineUnicodeTest_Simple::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	FLuaValue LuaValue = UnitTestState->RunString("return utf8.char(8704, 8712)", "");

	TestTrue(TEXT("LuaValue.String == TEXT(\"\u2200\u2208\")"), ULuaBlueprintFunctionLibrary::LuaValueToUTF8(LuaValue) == TEXT("\u2200\u2208"));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineUnicodeTest_Len, "LuaMachine.UnitTests.Unicode.Len", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineUnicodeTest_Len::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	UnitTestState->SetLuaValueFromGlobalName("test", ULuaBlueprintFunctionLibrary::LuaValueFromUTF8(TEXT("\u2200\u2208")));

	FLuaValue LuaValue = UnitTestState->RunString("return utf8.len(test)", "");

	TestTrue(TEXT("LuaValue.Integer == 2"), LuaValue.ToInteger() == 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineUnicodeTest_Codes, "LuaMachine.UnitTests.Unicode.Codes", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineUnicodeTest_Codes::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	UnitTestState->SetLuaValueFromGlobalName("test", ULuaBlueprintFunctionLibrary::LuaValueFromUTF8(TEXT("\u2200\u2208")));

	TArray<FLuaValue> LuaIter = UnitTestState->RunStringMulti("return utf8.codes(test)", "");

	TArray<FLuaValue> Results = UnitTestState->LuaValueCallMulti(LuaIter[0], { LuaIter[1], LuaIter[2] });

	TestTrue(TEXT("Iterator returned 2 values"), Results.Num() == 2);
	TestTrue(TEXT("UTF8 codepage 0 == 8704"), Results[1].ToInteger() == 8704);

	Results = UnitTestState->LuaValueCallMulti(LuaIter[0], { LuaIter[1], Results[0] });

	TestTrue(TEXT("Iterator returned 2 values"), Results.Num() == 2);
	TestTrue(TEXT("UTF8 codepage 1 == 8712"), Results[1].ToInteger() == 8712);

	Results = UnitTestState->LuaValueCallMulti(LuaIter[0], { LuaIter[1], Results[0] });

	TestTrue(TEXT("Iterator ended"), Results.IsEmpty());

	return true;
}


#endif