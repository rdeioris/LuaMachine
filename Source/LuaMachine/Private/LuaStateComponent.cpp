// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaStateComponent.h"


ULuaStateComponent::ULuaStateComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

ULuaState* ULuaStateComponent::GetLuaState()
{
	if (LuaState)
	{
		return LuaState;
	}

	if (!LuaStateClass)
	{
		return nullptr;
	}

	LuaState = NewObject<ULuaState>(this, LuaStateClass);
	LuaState->GetLuaState(GetWorld());
	LuaState->ReceiveLuaLevelAddedToWorld(GetOwner()->GetLevel(), GetWorld());
	return LuaState;
}

void ULuaStateComponent::DestroyLuaState()
{
	if (LuaState)
	{
		LuaState->ReceiveLuaLevelRemovedFromWorld(GetOwner()->GetLevel(), GetWorld());
		LuaState->MarkPendingKill();
		LuaState = nullptr;
	}
}

