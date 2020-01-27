// Copyright 2018-2020 - Roberto De Ioris

#include "LuaUserDataObject.h"

TSubclassOf<ULuaState> ULuaUserDataObject::GetLuaState() const
{
	ULuaState* LuaState = Cast<ULuaState>(GetOuter());
	if (LuaState)
	{
		return LuaState->GetClass();
	}
	return nullptr;
}

UWorld* ULuaUserDataObject::GetWorld() const
{
	ULuaState* LuaState = Cast<ULuaState>(GetOuter());
	if (LuaState)
	{
		return LuaState->GetWorld();
	}
	return nullptr;
}