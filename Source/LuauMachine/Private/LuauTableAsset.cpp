// Copyright 2018-2023 - Roberto De Ioris
// Portions Copyright 2025 - Fusion Point Studios, Inc.

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