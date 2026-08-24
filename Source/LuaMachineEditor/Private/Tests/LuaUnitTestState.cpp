// Copyright 2025 - Roberto De Ioris

#include "Tests/LuaUnitTestState.h"

FLuaValue ULuaUnitTestState::DummyFunction()
{
	return "Hello Test";
}

FLuaValue ULuaUnitTestState::ReturnTableFunction()
{
	return CreateLuaTable();
}