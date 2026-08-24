// Copyright 2025 - Roberto De Ioris

#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/LuaUnitTestState.h"
#include "Tests/LuaUserDataTest.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineUserDataInterfaceTest_Index, "LuaMachine.UnitTests.UserDataInterface.Index", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineUserDataInterfaceTest_Index::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	ULuaUserDataTest* TestObject = NewObject<ULuaUserDataTest>();

	TestObject->LuaUserDataEntries.Add("testvalue", []()
		{
			return "i am a test string";
		});

	FLuaValue LuaFunction = UnitTestState->RunString("return function(a) return a.testvalue end", "");

	TestTrue(TEXT("LuaValue.String == \"i am a test string\""), UnitTestState->LuaValueCall(LuaFunction, { TestObject }).String == "i am a test string");

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineUserDataInterfaceTest_NewIndex, "LuaMachine.UnitTests.UserDataInterface.NewIndex", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineUserDataInterfaceTest_NewIndex::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	ULuaUserDataTest* TestObject = NewObject<ULuaUserDataTest>();

	FLuaValue LuaFunction = UnitTestState->RunString("return function(a, b) a.testvalue = b; return a.testvalue end", "");

	TestTrue(TEXT("LuaValue.Integer == 17"), UnitTestState->LuaValueCall(LuaFunction, { TestObject, 17 }).Integer == 17);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineUserDataInterfaceTest_UFunction, "LuaMachine.UnitTests.UserDataInterface.UFunction", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineUserDataInterfaceTest_UFunction::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	UnitTestState->bLogError = true;

	ULuaUserDataTest* TestObject = NewObject<ULuaUserDataTest>();

	TestObject->LuaUserDataEntries.Add("dummy", []()
		{
			return FLuaValue::Function(GET_FUNCTION_NAME_CHECKED(ULuaUserDataTest, DummyString));
		});

	UnitTestState->SetLuaValueFromGlobalName("testobject", TestObject);

	FLuaValue ReturnValue = UnitTestState->RunString("return testobject.dummy()", "");

	TestTrue(TEXT("LuaValue.String == \"Dummy\""), ReturnValue.String == "Dummy");

	return true;
}


#endif