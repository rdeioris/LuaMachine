// Copyright 2025 - Roberto De Ioris

#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/LuaUnitTestState.h"
#include "Tests/LuaBlueprintPackageTest.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineBlueprintPackageTest_Simple, "LuaMachine.UnitTests.BlueprintPackage.Simple", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineBlueprintPackageTest_Simple::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	UnitTestState->RequireLuaBlueprintPackage<ULuaBlueprintPackageTest>("test");

	FLuaValue LuaTable = UnitTestState->RunString("return {x=test.test_number, y=test.test_bool, z=test.test_string, w=test.double_number(1000) }", "");

	TestTrue(TEXT("LuaTable.x == 17"), LuaTable.GetField("x").Integer == 17);
	TestTrue(TEXT("LuaTable.y == true"), LuaTable.GetField("y").Bool == true);
	TestTrue(TEXT("LuaTable.z == \"test\""), LuaTable.GetField("z").String == "test");
	TestTrue(TEXT("LuaTable.w == 2000"), LuaTable.GetField("w").Integer == 2000);

	return true;
}

#endif