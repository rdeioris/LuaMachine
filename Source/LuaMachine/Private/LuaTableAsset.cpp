// Copyright 2018-2023 - Roberto De Ioris

#include "LuaTableAsset.h"

FLuaValue ULuaTableAsset::ToLuaTable(ULuaState* LuaState)
{
	FLuaValue NewTable = LuaState->CreateLuaTable();
	for (TPair<FString, FLuaValue>& Pair : Table)
	{
		NewTable.SetField(Pair.Key, Pair.Value);
	}

	return NewTable;
}