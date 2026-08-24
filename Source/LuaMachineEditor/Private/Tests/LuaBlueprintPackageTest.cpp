// Copyright 2025 - Roberto De Ioris


#include "Tests/LuaBlueprintPackageTest.h"

ULuaBlueprintPackageTest::ULuaBlueprintPackageTest()
{
	Table.Add("test_bool", false);
}

void ULuaBlueprintPackageTest::Init()
{
	Table.Add("test_number", 17);
	Table.Add("test_bool", true);
	Table.Add("test_string", "test");
	Table.Add("double_number", FLuaValue::Function(GET_FUNCTION_NAME_CHECKED(ULuaBlueprintPackageTest, DoubleNumber)));
}

FLuaValue ULuaBlueprintPackageTest::DoubleNumber(FLuaValue LuaValue)
{
	return LuaValue.ToInteger() * 2;
}