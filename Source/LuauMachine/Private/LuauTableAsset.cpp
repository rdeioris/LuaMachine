// Copyright 2018-2023 - Roberto De Ioris

#include "LuauTableAsset.h"

FLuauValue ULuauTableAsset::ToLuauTable(ULuauState* LuauState)
{
	FLuauValue NewTable = LuauState->CreateLuauTable();
	for (TPair<FString, FLuauValue>& Pair : Table)
	{
		NewTable.SetField(Pair.Key, Pair.Value);
	}

	return NewTable;
}