// Copyright 2025 - Roberto De Ioris

#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/LuaUnitTestState.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineBinaryTest_Simple, "LuaMachine.UnitTests.Binary.Simple", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineBinaryTest_Simple::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	UnitTestState->SetLuaValueFromGlobalName("test", FLuaValue(TArray<uint8>({ 100, 200, 201 })));

	FLuaValue LuaValue = UnitTestState->RunString("return test", "");

	TestTrue(TEXT("LuaValue.ToBytes() == {100, 200, 201}"), LuaValue.ToBytes() == TArray<uint8>({ 100, 200, 201 }));

	return true;
}



#endif