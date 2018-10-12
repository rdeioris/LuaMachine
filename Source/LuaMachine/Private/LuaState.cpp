// Fill out your copyright notice in the Description page of Project Settings.

#include "LuaState.h"
#include "LuaComponent.h"
#include "GameFramework/Actor.h"

ULuaState::ULuaState()
{
	L = nullptr;
	bLuaOpenLibs = true;
	bDisabled = false;
	bLogError = true;
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

	ULuaState** LuaExtraSpacePtr = (ULuaState**)lua_getextraspace(L);
	*LuaExtraSpacePtr = this;
	// get the global table
	lua_pushglobaltable(L);
	// override print
	PushCFunction(ULuaState::TableFunction_print);
	SetField(-2, "print");

	// assign global symbols to nil, this will allow to override global functions/symbols
	for (TPair<FString, FLuaValue>& Pair : Table)
	{
		PushNil();
		SetField(-2, TCHAR_TO_UTF8(*Pair.Key));
	}


	// metatable
	NewTable();
	PushCFunction(MetaTableFunctionState__index);
	SetField(-2, "__index");
	PushCFunction(MetaTableFunctionState__newindex);
	SetField(-2, "__newindex");
	SetMetaTable(-2);

	Pop();

	const TCHAR* CodeRaw = *(LuaCodeAsset->Code.ToString());
	FString CodePath = FString("@") + LuaCodeAsset->GetPathName();

	if (luaL_loadbuffer(L, TCHAR_TO_UTF8(CodeRaw), FCStringAnsi::Strlen(TCHAR_TO_UTF8(CodeRaw)), TCHAR_TO_UTF8(*CodePath)))
	{
		LogError(FString::Printf(TEXT("Lua loading error: %s"), UTF8_TO_TCHAR(lua_tostring(L, -1))));
		bDisabled = true;
		lua_close(L);
		L = nullptr;
		return nullptr;
	}
	else
	{
		if (lua_pcall(L, 0, 0, 0))
		{
			LogError(FString::Printf(TEXT("Lua compilation error: %s"), UTF8_TO_TCHAR(lua_tostring(L, -1))));
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
	UObject* Object = nullptr;

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
	case ELuaValueType::Object:
		Object = LuaValue.ObjectPath.ResolveObject();
		if (Object)
		{
			ULuaState* InLuaState = ULuaState::GetFromExtraSpace(L);
			ULuaComponent* LuaComponent = Cast<ULuaComponent>(Object);
			if (LuaComponent)
			{
				InLuaState->NewUObject(LuaComponent);
				if (LuaComponent->LuaState == InLuaState->GetClass())
				{
					LuaComponent->SetupMetatable();
					return;
				}
			}
			else {
				AActor* Actor = Cast<AActor>(Object);
				if (Actor)
				{
					TArray<UActorComponent*> LuaComponents = Actor->GetComponentsByClass(ULuaComponent::StaticClass());
					for (UActorComponent* Component : LuaComponents)
					{
						ULuaComponent* LuaComponent = Cast<ULuaComponent>(Component);
						if (LuaComponent->LuaState == InLuaState->GetClass())
						{
							InLuaState->NewUObject(LuaComponent);
							LuaComponent->SetupMetatable();
							return;
						}
					}
				}
			}
			InLuaState->NewUObject(Object);
			return;
		}
		lua_pushnil(L);
		break;
	default:
		lua_pushnil(L);
	}
}

FLuaValue ULuaState::ToLuaValue(int Index)
{
	FLuaValue LuaValue;
	Internal_ToLuaValue(L, &LuaValue, Index);
	return LuaValue;
}

void ULuaState::Internal_ToLuaValue(lua_State* L, FLuaValue* LuaValue, int Index)
{
	if (lua_isboolean(L, Index))
	{
		LuaValue->Type = ELuaValueType::Bool;
		LuaValue->Bool = lua_toboolean(L, Index) != 0;
	}
	else if (lua_isinteger(L, Index))
	{
		LuaValue->Type = ELuaValueType::Integer;
		LuaValue->Integer = lua_tointeger(L, Index);
	}
	else if (lua_isnumber(L, Index))
	{
		LuaValue->Type = ELuaValueType::Number;
		LuaValue->Number = lua_tonumber(L, Index);
	}
	else if (lua_isstring(L, Index))
	{
		LuaValue->Type = ELuaValueType::String;
		LuaValue->String = FString(UTF8_TO_TCHAR(lua_tostring(L, Index)));
	}
	else if (lua_istable(L, Index))
	{
		LuaValue->Type = ELuaValueType::Table;
	}
	else if (lua_isuserdata(L, Index))
	{
		FLuaUserData* UserData = (FLuaUserData*)lua_touserdata(L, Index);
		LuaValue->Type = UserData->Type;
		switch (UserData->Type)
		{
		case(ELuaValueType::Object):
			LuaValue->ObjectPath = FSoftObjectPath(UserData->Context);
			break;
		case(ELuaValueType::Function):
			LuaValue->FunctionName = UserData->Function->GetFName();
			break;
		}
	}
}

int ULuaState::MetaTableFunctionState__index(lua_State *L)
{

	ULuaState* LuaState = ULuaState::GetFromExtraSpace(L);

	FString Key = UTF8_TO_TCHAR(lua_tostring(L, 2));

	FLuaValue* LuaValue = LuaState->Table.Find(Key);
	if (LuaValue)
	{
		// first check for UFunction
		if (LuaValue->Type == ELuaValueType::Function)
		{
			UFunction* Function = LuaState->FindFunction(LuaValue->FunctionName);
			if (Function)
			{
				FLuaUserData* LuaCallContext = (FLuaUserData*)lua_newuserdata(L, sizeof(FLuaUserData));
				LuaCallContext->Type = ELuaValueType::Function;
				LuaCallContext->Context = LuaState;
				LuaCallContext->Function = Function;
				LuaState->NewTable();
				LuaState->PushCFunction(ULuaState::MetaTableFunction__call);
				LuaState->SetField(-2, "__call");
				LuaState->SetMetaTable(-2);
				return 1;
			}
		}
		else {
			LuaState->FromLuaValue(*LuaValue);
			return 1;
		}
	}

	LuaState->PushNil();
	return 1;
}

int ULuaState::MetaTableFunctionState__newindex(lua_State *L)
{
	ULuaState* LuaState = ULuaState::GetFromExtraSpace(L);

	FString Key = UTF8_TO_TCHAR(lua_tostring(L, 2));

	FLuaValue* LuaValue = LuaState->Table.Find(Key);
	if (LuaValue)
	{
		LuaState->Internal_ToLuaValue(L, LuaValue, 3);
	}
	else
	{
		lua_rawset(L, 1);
	}

	return 0;
}

int ULuaState::MetaTableFunctionLuaComponent__index(lua_State *L)
{
	ULuaState* LuaState = ULuaState::GetFromExtraSpace(L);

	if (!lua_isuserdata(L, 1))
	{
		return luaL_error(L, "invalid state for ULuaComponent");
	}
	FLuaUserData* UserData = (FLuaUserData*)lua_touserdata(L, 1);
	ULuaComponent* LuaComponent = (ULuaComponent*)UserData->Context;

	FString Key = UTF8_TO_TCHAR(lua_tostring(L, 2));

	FLuaValue* LuaValue = LuaComponent->Table.Find(Key);
	if (LuaValue)
	{
		// first check for UFunction
		if (LuaValue->Type == ELuaValueType::Function)
		{
			UFunction* Function = LuaComponent->GetOwner()->FindFunction(LuaValue->FunctionName);
			if (Function)
			{
				FLuaUserData* LuaCallContext = (FLuaUserData*)lua_newuserdata(L, sizeof(FLuaUserData));
				LuaCallContext->Type = ELuaValueType::Function;
				LuaCallContext->Context = LuaComponent->GetOwner();
				LuaCallContext->Function = Function;
				LuaState->NewTable();
				LuaState->PushCFunction(ULuaState::MetaTableFunction__call);
				LuaState->SetField(-2, "__call");
				LuaState->SetMetaTable(-2);
				return 1;
			}
		}
		else {
			LuaState->FromLuaValue(*LuaValue);
			return 1;
		}
	}

	LuaState->PushNil();
	return 1;
}

int ULuaState::MetaTableFunctionLuaComponent__newindex(lua_State *L)
{
	ULuaState* LuaState = ULuaState::GetFromExtraSpace(L);

	if (!lua_isuserdata(L, 1))
	{
		return luaL_error(L, "invalid state for ULuaComponent");
	}
	FLuaUserData* UserData = (FLuaUserData*)lua_touserdata(L, 1);
	ULuaComponent* LuaComponent = (ULuaComponent*)UserData->Context;

	FString Key = UTF8_TO_TCHAR(lua_tostring(L, 2));

	FLuaValue* LuaValue = LuaComponent->Table.Find(Key);
	if (LuaValue)
	{
		LuaState->Internal_ToLuaValue(L, LuaValue, 3);
	}
	else
	{
		lua_rawset(L, 1);
	}

	return 0;
}

int ULuaState::MetaTableFunction__call(lua_State *L)
{
	ULuaState* LuaState = ULuaState::GetFromExtraSpace(L);

	if (!lua_isuserdata(L, 1))
	{
		return luaL_error(L, "invalid state for lua UFunction");
	}
	FLuaUserData* LuaCallContext = (FLuaUserData*)lua_touserdata(L, 1);

	FScopeCycleCounterUObject ObjectScope(LuaCallContext->Context);
	FScopeCycleCounterUObject FunctionScope(LuaCallContext->Function);

	void* Parameters = FMemory_Alloca(LuaCallContext->Function->ParmsSize);
	FMemory::Memzero(Parameters, LuaCallContext->Function->ParmsSize);

	int StackPointer = 2;
	// arguments
	for (TFieldIterator<UProperty> FArgs(LuaCallContext->Function); FArgs && ((FArgs->PropertyFlags & (CPF_Parm | CPF_ReturnParm)) == CPF_Parm); ++FArgs)
	{
		UProperty *Prop = *FArgs;
		UStructProperty* LuaProp = Cast<UStructProperty>(Prop);
		if (!LuaProp)
			continue;
		if (LuaProp->Struct != FLuaValue::StaticStruct())
			continue;

		FLuaValue LuaValue = LuaState->ToLuaValue(StackPointer++);
		*LuaProp->ContainerPtrToValuePtr<FLuaValue>(Parameters) = LuaValue;
	}

	LuaCallContext->Context->ProcessEvent(LuaCallContext->Function, Parameters);

	// get return value
	for (TFieldIterator<UProperty> FArgs(LuaCallContext->Function); FArgs && (FArgs->PropertyFlags & CPF_ReturnParm); ++FArgs)
	{
		UProperty *Prop = *FArgs;
		UStructProperty* LuaProp = Cast<UStructProperty>(Prop);
		if (!LuaProp)
			continue;
		if (LuaProp->Struct != FLuaValue::StaticStruct())
			continue;

		FLuaValue* LuaValue = LuaProp->ContainerPtrToValuePtr<FLuaValue>(Parameters);
		if (LuaValue)
		{
			LuaState->FromLuaValue(*LuaValue);
			return 1;
		}
	}

	LuaState->PushNil();
	return 1;
}

int ULuaState::TableFunction_print(lua_State *L)
{
	ULuaState* LuaState = ULuaState::GetFromExtraSpace(L);
	TArray<FString> Messages;

	int n = lua_gettop(L);
	lua_getglobal(L, "tostring");
	for (int i = 1; i <= n; i++)
	{
		lua_pushvalue(L, -1);
		lua_pushvalue(L, i);
		FLuaValue Value;
		if (!LuaState->Call(1, Value))
		{
			return luaL_error(L, "%s", TCHAR_TO_UTF8(*LuaState->LastError));
		}
		LuaState->Pop();
		Messages.Add(Value.ToString());
	}
	LuaState->Log(FString::Join(Messages, TEXT("\t")));
	return 0;
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

void ULuaState::NewUObject(UObject* Object)
{
	FLuaUserData* UserData = (FLuaUserData*)lua_newuserdata(L, sizeof(FLuaUserData));
	UserData->Type = ELuaValueType::Object;
	UserData->Context = Object;
	UserData->Function = nullptr;
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

bool ULuaState::PCall(int NArgs, FLuaValue& Value, int NRet)
{
	bool bSuccess = Call(NArgs, Value, NRet);
	if (!bSuccess)
	{
		if (bLogError)
			LogError(LastError);
		ReceiveLuaError(LastError);
	}
	return bSuccess;
}

bool ULuaState::Call(int NArgs, FLuaValue& Value, int NRet)
{
	if (lua_pcall(L, NArgs, NRet, 0))
	{
		LastError = UTF8_TO_TCHAR(lua_tostring(L, -1));
		return false;
	}
	if (NRet > 0)
		Value = ToLuaValue(-1);
	return true;
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

void* ULuaState::NewUserData(size_t DataSize)
{
	return lua_newuserdata(L, DataSize);
}

ULuaState::~ULuaState()
{
	if (L)
		lua_close(L);
}

