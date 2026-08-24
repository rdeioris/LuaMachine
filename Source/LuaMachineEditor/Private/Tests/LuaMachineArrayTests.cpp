// Copyright 2025 - Roberto De Ioris

#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/LuaUnitTestState.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineArrayTest_GetFieldByIndex, "LuaMachine.UnitTests.Array.GetFieldByIndex", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineArrayTest_GetFieldByIndex::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	FLuaValue LuaValue = UnitTestState->RunString("return { 100 }", "");

	TestTrue(TEXT("#LuaValue == 1"), UnitTestState->LuaValueLength(LuaValue) == 1);
	TestTrue(TEXT("LuaValue[1] == 100"), LuaValue.GetFieldByIndex(1).ToInteger() == 100);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineArrayTest_SetFieldByIndex, "LuaMachine.UnitTests.Array.SetFieldByIndex", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineArrayTest_SetFieldByIndex::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	FLuaValue LuaArray = UnitTestState->CreateLuaTable();

	LuaArray.SetFieldByIndex(1, 1000);
	LuaArray.SetFieldByIndex(2, 2000);
	LuaArray.SetFieldByIndex(3, 3000);

	TestTrue(TEXT("#LuaArray == 3"), UnitTestState->LuaValueLength(LuaArray) == 3);
	TestTrue(TEXT("LuaArray[1] == 1000"), LuaArray.GetFieldByIndex(1).ToInteger() == 1000);
	TestTrue(TEXT("LuaArray[2] == 2000"), LuaArray.GetFieldByIndex(2).ToInteger() == 2000);
	TestTrue(TEXT("LuaArray[3] == 3000"), LuaArray.GetFieldByIndex(3).ToInteger() == 3000);

	return true;
}

#endif