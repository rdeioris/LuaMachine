// Copyright 2025 - Roberto De Ioris

#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/LuaUnitTestState.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineTableTest_GetField, "LuaMachine.UnitTests.Table.GetField", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineTableTest_GetField::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	UnitTestState->SetLuaValueFromGlobalName("test", UnitTestState->CreateLuaTable());

	FLuaValue LuaValue = UnitTestState->RunString("test.x = 100; return test", "");

	TestTrue(TEXT("LuaValue.x == 100"), LuaValue.GetField("x").ToInteger() == 100);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineTableTest_SetField, "LuaMachine.UnitTests.Table.SetField", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineTableTest_SetField::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	FLuaValue LuaTable = UnitTestState->CreateLuaTable();

	LuaTable.SetField("x", 1000);
	LuaTable.SetField("y", "test");
	LuaTable.SetField("z", true);


	TestTrue(TEXT("LuaValue.x == 100"), LuaTable.GetField("x").ToInteger() == 1000);
	TestTrue(TEXT("LuaValue.y == \"test\""), LuaTable.GetField("y").ToString() == "test");
	TestTrue(TEXT("LuaValue.z == true"), LuaTable.GetField("z").ToBool());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineTableTest_Call, "LuaMachine.UnitTests.Table.Call", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineTableTest_Call::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	FLuaValue CallMe = UnitTestState->RunString("return {callme = function() return 17 end}", "");

	TestTrue(TEXT("LuaValue == 17"), UnitTestState->LuaValueCall(CallMe.GetField("callme"), {}).ToInteger() == 17);

	return true;
}

#endif
