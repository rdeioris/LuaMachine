// Fill out your copyright notice in the Description page of Project Settings.

#include "LuaState.h"
#include "LuaComponent.h"
#include "GameFramework/Actor.h"
#include "Runtime/Core/Public/Misc/FileHelper.h"

ULuaState::ULuaState()
{
	L = nullptr;
	bLuaOpenLibs = true;
	bDisabled = false;
	bLogError = true;

}

ULuaState* ULuaState::GetLuaState(UWorld* InWorld)
{
	CurrentWorld = InWorld;

	if (L != nullptr)
	{
		return this;
	}

	if (bDisabled)
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
	GetField(-1, "package");
	if (!OverridePackagePath.IsEmpty())
	{
		lua_pushstring(L, TCHAR_TO_UTF8(*OverridePackagePath));
		SetField(-2, "path");
	}
	if (!OverridePackageCPath.IsEmpty())
	{
		lua_pushstring(L, TCHAR_TO_UTF8(*OverridePackageCPath));
		SetField(-2, "cpath");
	}
	// manage RequireTable
	GetField(-1, "preload");
	for (TPair<FString, ULuaCode*>& Pair : RequireTable)
	{
		PushCFunction(ULuaState::TableFunction_package_preload);
		SetField(-2, TCHAR_TO_UTF8(*Pair.Key));
	}

	// pop package.preload
	Pop(2);

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

	// pop global table
	Pop();

	if (LuaCodeAsset)
	{

		if (!RunCode(LuaCodeAsset->Code.ToString(), LuaCodeAsset->GetPathName()))
		{
			if (bLogError)
				LogError(LastError);
			ReceiveLuaError(LastError);
			bDisabled = true;
			lua_close(L);
			L = nullptr;
			return nullptr;
		}
	}

	if (!LuaFilename.IsEmpty())
	{
		FString Code;
		if (FFileHelper::LoadFileToString(Code, *LuaFilename))
		{
			if (!RunCode(Code, LuaFilename))
			{
				if (bLogError)
					LogError(LastError);
				ReceiveLuaError(LastError);
				bDisabled = true;
				lua_close(L);
				L = nullptr;
				return nullptr;
			}
		}
	}

	return this;
}

bool ULuaState::RunCode(FString Code, FString CodePath, int NRet)
{
	const TCHAR* CodeRaw = *Code;
	FString FullCodePath = FString("@") + CodePath;

	if (luaL_loadbuffer(L, TCHAR_TO_UTF8(CodeRaw), FCStringAnsi::Strlen(TCHAR_TO_UTF8(CodeRaw)), TCHAR_TO_UTF8(*FullCodePath)))
	{
		LastError = FString::Printf(TEXT("Lua loading error: %s"), UTF8_TO_TCHAR(lua_tostring(L, -1)));
		return false;
	}
	else
	{

		if (lua_pcall(L, 0, NRet, 0))
		{
			LastError = FString::Printf(TEXT("Lua compilation error: %s"), UTF8_TO_TCHAR(lua_tostring(L, -1)));
			return false;
		}
	}

	return true;
}

void ULuaState::FromLuaValue(FLuaValue& LuaValue, bool bGenerateTableRef)
{
	Internal_FromLuaValue(L, LuaValue, bGenerateTableRef);
}

void ULuaState::Internal_FromLuaValue(lua_State* L, FLuaValue& LuaValue, bool bGenerateTableRef)
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
	case ELuaValueType::Table:
		if (LuaValue.TableRef == LUA_NOREF && bGenerateTableRef)
		{
			lua_newtable(L);
			LuaValue.TableRef = luaL_ref(L, LUA_REGISTRYINDEX);
		}
		lua_rawgeti(L, LUA_REGISTRYINDEX, LuaValue.TableRef);
		break;
	case ELuaValueType::Object:
	{
		ULuaState* InLuaState = ULuaState::GetFromExtraSpace(L);
		ULuaComponent* LuaComponent = Cast<ULuaComponent>(LuaValue.Object);
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
			AActor* Actor = Cast<AActor>(LuaValue.Object);
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
		InLuaState->NewUObject(LuaValue.Object);
		break;
	}
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

int32 ULuaState::GetTop()
{
	return lua_gettop(L);
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
			LuaValue->Object = UserData->Context;
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
			LuaState->FromLuaValue(*LuaValue, true);
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
			LuaState->FromLuaValue(*LuaValue, true);
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

	LuaState->InceptionLevel++;
	UE_LOG(LogLuaMachine, Error, TEXT("Inception Level: %d"), LuaState->InceptionLevel);
	LuaCallContext->Context->ProcessEvent(LuaCallContext->Function, Parameters);
	check(LuaState->InceptionLevel > 0);
	LuaState->InceptionLevel--;

	if (LuaState->InceptionLevel == 0)
	{
		UE_LOG(LogLuaMachine, Error, TEXT("Starting Inception..."));
		FString Error;
		while (LuaState->InceptionErrors.Dequeue(Error))
		{
			ULuaComponent* LuaComponent = Cast <ULuaComponent>(LuaCallContext->Context);
			if (LuaComponent)
			{
				if (LuaComponent->bLogError)
				{
					LuaState->LogError(Error);
				}
				LuaComponent->OnLuaError.Broadcast(Error);
			}
			else
			{
				if (LuaState->bLogError)
				{
					LuaState->LogError(Error);
				}
				LuaState->ReceiveLuaError(Error);
			}
		}
		UE_LOG(LogLuaMachine, Error, TEXT("Ended Inception..."));
	}


	// get return value
	for (TFieldIterator<UProperty> FArgs(LuaCallContext->Function); FArgs; ++FArgs)
	{
		UProperty *Prop = *FArgs;
		if (!Prop->HasAnyPropertyFlags(CPF_ReturnParm))
			continue;
		
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

int ULuaState::TableFunction_package_preload(lua_State *L)
{
	ULuaState* LuaState = ULuaState::GetFromExtraSpace(L);

	FString Key = UTF8_TO_TCHAR(lua_tostring(L, 1));

	ULuaCode** LuaCodePtr = LuaState->RequireTable.Find(Key);
	if (!LuaCodePtr)
	{
		return luaL_error(L, "unable to find package %s", TCHAR_TO_UTF8(*Key));
	}

	ULuaCode* LuaCode = *LuaCodePtr;
	if (!LuaCode)
	{
		return luaL_error(L, "LuaCodeAsset not set for package %s", TCHAR_TO_UTF8(*Key));
	}

	if (!LuaState->RunCode(LuaCode->Code.ToString(), LuaCode->GetPathName(), 1))
	{
		return luaL_error(L, "%s", lua_tostring(L, -1));
	}

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

void ULuaState::PushGlobalTable()
{
	lua_pushglobaltable(L);
}

int32 ULuaState::GetFieldFromTree(FString Tree, bool bGlobal)
{
	TArray<FString> Parts;
	Tree.ParseIntoArray(Parts, TEXT("."));
	if (Parts.Num() == 0)
	{
		LastError = FString::Printf(TEXT("invalid Lua key: \"%s\""), *Tree);
		if (bLogError)
			LogError(LastError);
		ReceiveLuaError(LastError);
		PushNil();
		return 1;
	}

	int32 AdditionalPop = bGlobal ? 1 : 0;

	if (bGlobal)
	{
		PushGlobalTable();
	}
	int32 i;

	for (i = 0; i < Parts.Num(); i++)
	{
		GetField(-1, TCHAR_TO_UTF8(*Parts[i]));

		if (lua_isnil(L, -1))
		{
			if (i == Parts.Num() - 1)
			{
				return i + 1 + AdditionalPop;
			}
			LastError = FString::Printf(TEXT("unknown Lua key: \"%s\""), *Parts[i]);
			if (bLogError)
				LogError(LastError);
			ReceiveLuaError(LastError);
			return i + 1 + AdditionalPop;
		}
	}

	return i + AdditionalPop;
}

void ULuaState::SetFieldFromTree(FString Tree, FLuaValue& Value)
{
	int32 ItemsToPop = GetFieldFromTree(Tree);
	// invalid key
	if (ItemsToPop == 1)
	{
		Pop();
		return;
	}

	TArray<FString> Parts;
	Tree.ParseIntoArray(Parts, TEXT("."));

	Pop();
	FromLuaValue(Value);
	SetField(-2, TCHAR_TO_UTF8(*Parts.Last()));
	Pop(ItemsToPop - 1);
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
		if (InceptionLevel > 0)
		{
			InceptionErrors.Enqueue(LastError);
		}
		else
		{
			if (bLogError)
				LogError(LastError);
			ReceiveLuaError(LastError);
		}
	}
	return bSuccess;
}

bool ULuaState::Call(int NArgs, FLuaValue& Value, int NRet)
{
	if (lua_pcall(L, NArgs, NRet, 0))
	{
		LastError = FString::Printf(TEXT("Lua error: %s"), UTF8_TO_TCHAR(lua_tostring(L, -1)));
		return false;
	}
	if (NRet > 0)
	{
		Value = ToLuaValue(-1);
	}
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

void ULuaState::Unref(int Ref)
{
	luaL_unref(L, LUA_REGISTRYINDEX, Ref);
}

ULuaState::~ULuaState()
{
	if (L)
		lua_close(L);
}

