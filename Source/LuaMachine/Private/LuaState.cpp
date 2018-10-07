// Fill out your copyright notice in the Description page of Project Settings.

#include "LuaState.h"
#include "GameFramework/Actor.h"

ULuaState::ULuaState()
{
	L = nullptr;
	bLuaOpenLibs = true;
	bDisabled = false;
}

ULuaState* ULuaState::GetLuaState(UWorld* InWorld)
{
	if (L != nullptr)
	{
		CurrentWorld = InWorld;
		return this;
	}

	if (bDisabled)
		return nullptr;

	if (!LuaCodeAsset)
		return nullptr;

	L = luaL_newstate();

	if (bLuaOpenLibs)
		luaL_openlibs(L);

	FString LuaFunctionPrefix = FString(TEXT("lua_"));

	// assign global functions
	for (TFieldIterator<UFunction> Funcs(GetClass()); Funcs; ++Funcs)
	{
		UFunction *Function = *Funcs;
		if (Function->GetName().StartsWith(LuaFunctionPrefix, ESearchCase::CaseSensitive))
		{
			UFunction ** FunctionPtr = (UFunction **)lua_newuserdata(L, sizeof(UFunction **));
			*FunctionPtr = Function;
			NewTable();
			ULuaState **LuaStatePtr = (ULuaState **)lua_newuserdata(L, sizeof(ULuaState **));
			*LuaStatePtr = this;
			SetField(-2, "__unreal_state");
			PushCFunction(MetaTableFunctionState__call);
			SetField(-2, "__call");
			SetMetaTable(-2);
			FString FunctionName = Function->GetName();
			FunctionName.RemoveFromStart(LuaFunctionPrefix, ESearchCase::CaseSensitive);
			SetGlobal(TCHAR_TO_UTF8(*FunctionName));
		}
	}

	// assign global properties
	for (TFieldIterator<UProperty> Props(GetClass()); Props; ++Props)
	{
		UProperty* Prop = *Props;
		UStructProperty* LuaProp = Cast<UStructProperty>(Prop);
		if (!LuaProp)
			continue;
		if (LuaProp->Struct != FLuaValue::StaticStruct())
			continue;
		if (LuaProp->HasAnyPropertyFlags(CPF_DisableEditOnInstance))
			continue;
		FLuaValue* LuaValue = LuaProp->ContainerPtrToValuePtr<FLuaValue>(this);
		FromLuaValue(*LuaValue);
		SetGlobal(TCHAR_TO_UTF8(*LuaProp->GetName()));
	}

	// assign global table
	for (TPair<FString, FLuaValue> Pair : Table)
	{
		FromLuaValue(Pair.Value);
		SetGlobal(TCHAR_TO_UTF8(*Pair.Key));
	}

	UE_LOG(LogTemp, Warning, TEXT("Lua Machine initialized at %p"), L);
	if (luaL_loadstring(L, TCHAR_TO_UTF8(*LuaCodeAsset->Code)))
	{
		UE_LOG(LogTemp, Error, TEXT("Lua loading error: %s"), UTF8_TO_TCHAR(lua_tostring(L, -1)));
		bDisabled = true;
		lua_close(L);
		L = nullptr;
		return nullptr;
	}
	else
	{
		if (lua_pcall(L, 0, 0, 0))
		{
			UE_LOG(LogTemp, Error, TEXT("Lua compilation error: %s"), UTF8_TO_TCHAR(lua_tostring(L, -1)));
			bDisabled = true;
			lua_close(L);
			L = nullptr;
			return nullptr;
		}
	}

	return this;
}

void ULuaState::FromLuaValue(FLuaValue& LuaValue)
{
	Internal_FromLuaValue(L, LuaValue);
}

void ULuaState::Internal_FromLuaValue(lua_State* L, FLuaValue& LuaValue)
{
	switch (LuaValue.Type)
	{
	case ELuaValueType::Bool:
		lua_pushboolean(L, LuaValue.Bool ? 1 : 0);
		break;
	case ELuaValueType::Integer:
		lua_pushinteger(L, LuaValue.Integer);
		break;
	case ELuaValueType::Number:
		lua_pushnumber(L, LuaValue.Number);
		break;
	case ELuaValueType::String:
		lua_pushstring(L, TCHAR_TO_UTF8(*LuaValue.String));
		break;
	default:
		lua_pushnil(L);
	}
}

FLuaValue ULuaState::ToLuaValue(int Index)
{
	return Internal_ToLuaValue(L, Index);
}

FLuaValue ULuaState::Internal_ToLuaValue(lua_State* L, int Index)
{
	FLuaValue LuaValue;

	if (lua_isboolean(L, Index))
	{
		LuaValue.Type = ELuaValueType::Bool;
		LuaValue.Bool = lua_toboolean(L, Index) != 0;
	}
	else if (lua_isinteger(L, Index))
	{
		LuaValue.Type = ELuaValueType::Integer;
		LuaValue.Integer = lua_tointeger(L, Index);
	}
	else if (lua_isnumber(L, Index))
	{
		LuaValue.Type = ELuaValueType::Number;
		LuaValue.Number = lua_tonumber(L, Index);
	}
	else if (lua_isstring(L, Index))
	{
		LuaValue.Type = ELuaValueType::String;
		LuaValue.String = FString(UTF8_TO_TCHAR(lua_tostring(L, Index)));
	}

	return LuaValue;
}

int ULuaState::MetaTableFunctionActor__index(lua_State *L)
{
	lua_getfield(L, 1, "__unreal_actor");
	if (!lua_isuserdata(L, -1))
	{
		return luaL_error(L, "invalid state for __unreal_actor internal field");
	}
	AActor** unreal_actor_ptr = (AActor **)lua_touserdata(L, -1);
	AActor* Actor = *unreal_actor_ptr;
	const char *key = lua_tostring(L, 2);
	UE_LOG(LogTemp, Warning, TEXT("ASKING FOR %s"), UTF8_TO_TCHAR(key));
	// first find for a property
	UProperty* Prop = Actor->GetClass()->FindPropertyByName(FName(UTF8_TO_TCHAR(key)));
	if (Prop)
	{
		UStructProperty* StructProp = Cast<UStructProperty>(Prop);
		if (StructProp && StructProp->Struct == FLuaValue::StaticStruct() && !StructProp->HasAnyPropertyFlags(CPF_DisableEditOnInstance))
		{
			FLuaValue* LuaValue = StructProp->ContainerPtrToValuePtr<FLuaValue>(Actor);
			Internal_FromLuaValue(L, *LuaValue);
			return 1;
		}
	}
	else
	{
		FString FullKey = FString("lua_") + FString(UTF8_TO_TCHAR(key));
		UFunction* Function = Actor->GetClass()->FindFunctionByName(FName(*FullKey));
		if (Function && Function->HasAnyFunctionFlags(EFunctionFlags::FUNC_Public))
		{
			UFunction **unreal_function_ptr = (UFunction **)lua_newuserdata(L, sizeof(UFunction **));
			*unreal_function_ptr = Function;
			lua_newtable(L);
			AActor **unreal_actor_ptr = (AActor **)lua_newuserdata(L, sizeof(AActor **));
			*unreal_actor_ptr = Actor;
			lua_setfield(L, -2, "__unreal_actor");
			lua_pushcfunction(L, MetaTableFunctionActor__call);
			lua_setfield(L, -2, "__call");
			lua_setmetatable(L, -2);
			return 1;
		}
	}

	lua_pushnil(L);
	return 1;
}

int ULuaState::MetaTableFunctionActor__call(lua_State *L)
{
	if (!lua_isuserdata(L, 1))
	{
		return luaL_error(L, "invalid state for lua UFunction");
	}
	UFunction** unreal_function_ptr = (UFunction **)lua_touserdata(L, 1);
	UFunction* Function = *unreal_function_ptr;

	lua_getmetatable(L, 1);
	lua_getfield(L, -1, "__unreal_actor");
	if (!lua_isuserdata(L, -1))
	{
		return luaL_error(L, "invalid state for __unreal_actor internal field");
	}
	AActor** unreal_actor_ptr = (AActor **)lua_touserdata(L, -1);
	AActor* Actor = *unreal_actor_ptr;

	UE_LOG(LogTemp, Warning, TEXT("Ready to CALL function %s on actor %s !!!"), *Function->GetName(), *Actor->GetName());

	FScopeCycleCounterUObject ObjectScope(Actor);
	FScopeCycleCounterUObject FunctionScope(Function);

	void* Parameters = FMemory_Alloca(Function->ParmsSize);
	FMemory::Memzero(Parameters, Function->ParmsSize);

	int StackPointer = 2;
	// arguments
	for (TFieldIterator<UProperty> FArgs(Function); FArgs && ((FArgs->PropertyFlags & (CPF_Parm | CPF_ReturnParm)) == CPF_Parm); ++FArgs)
	{
		UProperty *Prop = *FArgs;
		UStructProperty* LuaProp = Cast<UStructProperty>(Prop);
		if (!LuaProp)
			continue;
		if (LuaProp->Struct != FLuaValue::StaticStruct())
			continue;
		UE_LOG(LogTemp, Warning, TEXT("Argument: %d %s"), StackPointer, *LuaProp->GetName());


		FLuaValue LuaValue = Internal_ToLuaValue(L, StackPointer++);

		*LuaProp->ContainerPtrToValuePtr<FLuaValue>(Parameters) = LuaValue;
	}

	Actor->ProcessEvent(Function, Parameters);


	lua_pushnil(L);
	return 1;
}


int ULuaState::MetaTableFunctionState__call(lua_State *L)
{
	if (!lua_isuserdata(L, 1))
	{
		return luaL_error(L, "invalid state for lua UFunction");
	}
	UFunction** unreal_function_ptr = (UFunction **)lua_touserdata(L, 1);
	UFunction* Function = *unreal_function_ptr;

	lua_getmetatable(L, 1);
	lua_getfield(L, -1, "__unreal_state");
	if (!lua_isuserdata(L, -1))
	{
		return luaL_error(L, "invalid state for __unreal_actor internal field");
	}
	ULuaState** LuaStatePtr = (ULuaState **)lua_touserdata(L, -1);
	ULuaState* LuaState = *LuaStatePtr;

	FScopeCycleCounterUObject ObjectScope(LuaState);
	FScopeCycleCounterUObject FunctionScope(Function);

	void* Parameters = FMemory_Alloca(Function->ParmsSize);
	FMemory::Memzero(Parameters, Function->ParmsSize);

	int StackPointer = 2;
	// arguments
	for (TFieldIterator<UProperty> FArgs(Function); FArgs && ((FArgs->PropertyFlags & (CPF_Parm | CPF_ReturnParm)) == CPF_Parm); ++FArgs)
	{
		UProperty *Prop = *FArgs;
		UStructProperty* LuaProp = Cast<UStructProperty>(Prop);
		if (!LuaProp)
			continue;
		if (LuaProp->Struct != FLuaValue::StaticStruct())
			continue;

		FLuaValue LuaValue = Internal_ToLuaValue(L, StackPointer++);
		*LuaProp->ContainerPtrToValuePtr<FLuaValue>(Parameters) = LuaValue;
	}

	LuaState->ProcessEvent(Function, Parameters);

	lua_pushnil(L);
	return 1;
}

void ULuaState::NewTable()
{
	lua_newtable(L);
}

void ULuaState::SetMetaTable(int Index)
{
	lua_setmetatable(L, Index);
}

void ULuaState::SetField(int Index, const char* FieldName)
{
	lua_setfield(L, Index, FieldName);
}

void ULuaState::GetField(int Index, const char* FieldName)
{
	lua_getfield(L, Index, FieldName);
}

void ULuaState::NewActor(AActor* Actor)
{
	AActor **ActorPtr = (AActor **)lua_newuserdata(L, sizeof(AActor **));
	*ActorPtr = Actor;
}

void ULuaState::GetGlobal(const char* Name)
{
	lua_getglobal(L, Name);
}

int32 ULuaState::GetFunctionFromTree(FString Tree)
{
	TArray<FString> Parts;
	Tree.ParseIntoArray(Parts, TEXT("."));
	if (Parts.Num() == 0)
		return 0;

	GetGlobal(TCHAR_TO_UTF8(*Parts[0]));
	if (lua_isnil(L, -1))
		return false;

	int32 i;

	for (i = 1; i < Parts.Num(); i++)
	{
		GetField(-1, TCHAR_TO_UTF8(*Parts[i]));

		if (lua_isnil(L, -1))
		{
			return -i;
		}
	}

	return i;
}

void ULuaState::SetGlobal(const char* Name)
{
	lua_setglobal(L, Name);
}

void ULuaState::PushValue(int Index)
{
	lua_pushvalue(L, Index);
}

FLuaValue ULuaState::PCall(int NArgs)
{
	if (lua_pcall(L, NArgs, 1, 0))
	{
		return FLuaValue(ELuaValueType::Error, UTF8_TO_TCHAR(lua_tostring(L, -1)));
	}
	return ToLuaValue(-1);
}

void ULuaState::Pop(int32 Amount)
{
	lua_pop(L, Amount);
}

void ULuaState::PushNil()
{
	lua_pushnil(L);
}

void ULuaState::PushCFunction(lua_CFunction Function)
{
	lua_pushcfunction(L, Function);
}


ULuaState::~ULuaState()
{
	if (L)
		lua_close(L);
}

