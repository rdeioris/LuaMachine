// Copyright 2025 - Roberto De Ioris


#include "Tests/LuaUserDataTest.h"

FLuaValue ULuaUserDataTest::LuaMetaMethodIndex_Implementation(const FString& Key)
{
	if (!LuaUserDataEntries.Contains(Key))
	{
		return FLuaValue();
	}

	return LuaUserDataEntries[Key]();
}

bool ULuaUserDataTest::LuaMetaMethodNewIndex_Implementation(const FString& Key, FLuaValue LuaValue)
{
	if (LuaUserDataEntries.Contains(Key))
	{
		return false;
	}

	LuaUserDataEntries.Add(Key, [LuaValue]() { return LuaValue; });

	return true;
}