// Copyright 2019 - Roberto De Ioris

#include "LuaState.h"
#include "LuaComponent.h"
#include "LuaMachine.h"
#include "GameFramework/Actor.h"
#include "Runtime/Core/Public/Misc/FileHelper.h"
#include "Runtime/Core/Public/Misc/Paths.h"
#include "Runtime/Core/Public/Serialization/BufferArchive.h"
#include "Runtime/CoreUObject/Public/UObject/TextProperty.h"
#include "Runtime/Engine/Classes/Kismet/BlueprintFunctionLibrary.h"

LUAMACHINE_API DEFINE_LOG_CATEGORY(LogLuaMachine);

ULuaState::ULuaState()
{
	L = nullptr;
	bLuaOpenLibs = true;
	bDisabled = false;
	bLogError = true;
	bAddProjectContentDirToPackagePath = true;
	bPersistent = false;
	bEnableLineHook = false;
	bEnableCallHook = false;
	bEnableReturnHook = false;
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

	if (bAddProjectContentDirToPackagePath)
	{
		GetField(-1, "path");
		const char* CurrentLuaPath = lua_tostring(L, -1);
		FString NewPackagePath = FString(CurrentLuaPath) + ";" + FPaths::ProjectContentDir() + "/?.lua";
		Pop();
		lua_pushstring(L, TCHAR_TO_UTF8(*NewPackagePath));
		SetField(-2, "path");
	}

	for (FString SubDir : AppendProjectContentDirSubDir)
	{
		GetField(-1, "path");
		const char* CurrentLuaPath = lua_tostring(L, -1);
		FString NewPackagePath = FString(CurrentLuaPath) + ";" + FPaths::ProjectContentDir() / SubDir + "/?.lua";
		Pop();
		lua_pushstring(L, TCHAR_TO_UTF8(*NewPackagePath));
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
	for (TPair<FString, TSubclassOf<UBlueprintFunctionLibrary>>& Pair : RequireBlueprintFunctionLibraryTable)
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

	int DebugMask = 0;
	// install hooks
	if (bEnableLineHook)
	{
		DebugMask |= LUA_MASKLINE;
	}
	if (bEnableCallHook)
	{
		DebugMask |= LUA_MASKCALL;
	}
	if (bEnableReturnHook)
	{
		DebugMask |= LUA_MASKRET;
	}

	if (DebugMask != 0)
	{
		lua_sethook(L, Debug_Hook, DebugMask, 0);
	}

	if (LuaCodeAsset)
	{
		if (!RunCodeAsset(LuaCodeAsset))
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
		if (!RunFile(LuaFilename, true))
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

	if (UserDataMetaTableFromCodeAsset)
	{
		if (!RunCodeAsset(UserDataMetaTableFromCodeAsset, 1))
		{
			if (bLogError)
				LogError(LastError);
			ReceiveLuaError(LastError);
			bDisabled = true;
			lua_close(L);
			L = nullptr;
			return nullptr;
		}
		UserDataMetaTable = ToLuaValue(-1);
		Pop();
	}

	return this;
}

bool ULuaState::RunCodeAsset(ULuaCode* CodeAsset, int NRet)
{

	if (CodeAsset->bCooked && CodeAsset->bCookAsBytecode)
	{
#if PLATFORM_ANDROID
		// fix size_t of the bytecode
		if (CodeAsset->ByteCode.Num() >= 14)
			CodeAsset->ByteCode[13] = sizeof(size_t);
#endif
		return RunCode(CodeAsset->ByteCode, CodeAsset->GetPathName(), NRet);
	}

	return RunCode(CodeAsset->Code.ToString(), CodeAsset->GetPathName(), NRet);

}

bool ULuaState::RunFile(FString Filename, bool bIgnoreNonExistent, int NRet)
{
	TArray<uint8> Code;
	FString AbsoluteFilename = FPaths::Combine(FPaths::ProjectContentDir(), Filename);

	if (!FPaths::FileExists(AbsoluteFilename))
	{
		if (bIgnoreNonExistent)
			return true;
		LastError = FString::Printf(TEXT("Unable to open file %s"), *Filename);
		FLuaValue LuaLastError = FLuaValue(LastError);
		FromLuaValue(LuaLastError);
		return false;
	}

	if (FFileHelper::LoadFileToArray(Code, *AbsoluteFilename))
	{
		if (RunCode(Code, AbsoluteFilename, NRet))
		{
			return true;
		}
		return false;
	}

	LastError = FString::Printf(TEXT("Unable to open file %s"), *Filename);
	FLuaValue LuaLastError = FLuaValue(LastError);
	FromLuaValue(LuaLastError);
	return false;
}

bool ULuaState::RunCode(FString Code, FString CodePath, int NRet)
{
	TArray<uint8> Bytes;
	Bytes.Append((uint8 *)TCHAR_TO_UTF8(*Code), FCStringAnsi::Strlen(TCHAR_TO_UTF8(*Code)));
	return RunCode(Bytes, CodePath, NRet);
}

bool ULuaState::RunCode(TArray<uint8> Code, FString CodePath, int NRet)
{
	FString FullCodePath = FString("@") + CodePath;

	if (luaL_loadbuffer(L, (const char *)Code.GetData(), Code.Num(), TCHAR_TO_UTF8(*FullCodePath)))
	{
		LastError = FString::Printf(TEXT("Lua loading error: %s"), UTF8_TO_TCHAR(lua_tostring(L, -1)));
		return false;
	}
	else
	{

		if (lua_pcall(L, 0, NRet, 0))
		{
			LastError = FString::Printf(TEXT("Lua execution error: %s"), UTF8_TO_TCHAR(lua_tostring(L, -1)));
			return false;
		}
	}

	return true;
}

int ULuaState::ToByteCode_Writer(lua_State* L, const void* Ptr, size_t Size, void* UserData)
{
	TArray<uint8>* Output = (TArray<uint8> *)UserData;
	Output->Append((uint8 *)Ptr, Size);
	return 0;
}

TArray<uint8> ULuaState::ToByteCode(FString Code, FString CodePath, FString& ErrorString)
{
	const TCHAR* CodeRaw = *Code;
	FString FullCodePath = FString("@") + CodePath;
	TArray<uint8> Output;

	lua_State* L = luaL_newstate();
	if (luaL_loadbuffer(L, TCHAR_TO_UTF8(CodeRaw), FCStringAnsi::Strlen(TCHAR_TO_UTF8(CodeRaw)), TCHAR_TO_UTF8(*FullCodePath)))
	{
		ErrorString = UTF8_TO_TCHAR(lua_tostring(L, -1));
		Output.Empty();
		lua_close(L);
		return Output;
	}

	if (lua_dump(L, ULuaState::ToByteCode_Writer, &Output, 1))
	{
		ErrorString = UTF8_TO_TCHAR(lua_tostring(L, -1));
		Output.Empty();
		lua_close(L);
		return Output;
	}

	lua_close(L);
	return Output;
}

void ULuaState::FromLuaValue(FLuaValue& LuaValue, UObject* CallContext, lua_State* State)
{
	if (!State)
		State = this->L;

	switch (LuaValue.Type)
	{
	case ELuaValueType::Bool:
		lua_pushboolean(State, LuaValue.Bool ? 1 : 0);
		break;
	case ELuaValueType::Integer:
		lua_pushinteger(State, LuaValue.Integer);
		break;
	case ELuaValueType::Number:
		lua_pushnumber(State, LuaValue.Number);
		break;
	case ELuaValueType::String:
		lua_pushstring(State, TCHAR_TO_UTF8(*LuaValue.String));
		break;
	case ELuaValueType::Table:
		if (LuaValue.LuaRef == LUA_NOREF)
		{
			lua_newtable(State);
			lua_pushvalue(State, -1);
			// hold references in the main state
			LuaValue.LuaRef = luaL_ref(this->L, LUA_REGISTRYINDEX);
			LuaValue.LuaState = this;
			break;
		}
		if (this != LuaValue.LuaState)
		{
			lua_pushnil(State);
			break;
		}
		lua_rawgeti(this->L, LUA_REGISTRYINDEX, LuaValue.LuaRef);
		if (this->L != State)
			lua_xmove(this->L, State, 1);
		break;
	case ELuaValueType::Thread:
		if (LuaValue.LuaRef == LUA_NOREF)
		{
			lua_newthread(State);
			lua_pushvalue(State, -1);
			LuaValue.LuaRef = luaL_ref(this->L, LUA_REGISTRYINDEX);
			LuaValue.LuaState = this;
			break;
		}
		if (this != LuaValue.LuaState)
		{
			lua_pushnil(State);
			break;
		}
		lua_rawgeti(this->L, LUA_REGISTRYINDEX, LuaValue.LuaRef);
		if (this->L != State)
			lua_xmove(this->L, State, 1);
		break;
	case ELuaValueType::Function:
		if (this != LuaValue.LuaState || LuaValue.LuaRef == LUA_NOREF)
		{
			lua_pushnil(State);
			break;
		}
		lua_rawgeti(this->L, LUA_REGISTRYINDEX, LuaValue.LuaRef);
		if (this->L != State)
			lua_xmove(this->L, State, 1);
		break;
	case ELuaValueType::UObject:
	{
		NewUObject(LuaValue.Object);
		ULuaComponent* LuaComponent = Cast<ULuaComponent>(LuaValue.Object);
		if (LuaComponent)
		{
			// ensure we are in the same LuaState
			if (LuaComponent->LuaState == GetClass())
			{
				LuaComponent->SetupMetatable(State);
			}
		}
		else {
			bool bHasMetaTable = false;
			if (UserDataMetaTable.Type == ELuaValueType::Table)
			{
				FromLuaValue(UserDataMetaTable, nullptr, State);
				lua_setmetatable(State, -2);
				lua_getmetatable(State, -1);
				bHasMetaTable = true;
			}
			else
			{
				lua_newtable(State);
			}
			// allow comparison between userdata/UObject/UFunction
			lua_pushcfunction(State, ULuaState::MetaTableFunctionUserData__eq);
			lua_setfield(State, -2, "__eq");

			if (bHasMetaTable)
			{
				lua_pop(State, 1);
			}
			else
			{
				lua_setmetatable(State, -2);
			}
		}
	}
	break;
	case ELuaValueType::UFunction:
		// if no context is assigned to the function, own it !
		if (!LuaValue.LuaState)
		{
			LuaValue.LuaState = this;
		}
		if (this != LuaValue.LuaState)
		{
			lua_pushnil(State);
			break;
		}
		// first time we should have a CallContext, then we cache it in the Object field
		if (!CallContext)
		{
			CallContext = LuaValue.Object;
		}
		if (CallContext)
		{
			UFunction* Function = CallContext->FindFunction(LuaValue.FunctionName);
			if (Function)
			{
				// cache it for context-less calls
				LuaValue.Object = CallContext;
				FLuaUserData* LuaCallContext = (FLuaUserData*)lua_newuserdata(State, sizeof(FLuaUserData));
				LuaCallContext->Type = ELuaValueType::UFunction;
				LuaCallContext->Context = CallContext;
				LuaCallContext->Function = Function;
				lua_newtable(State);
				lua_pushcfunction(State, ULuaState::MetaTableFunction__call);
				lua_setfield(State, -2, "__call");
				lua_setmetatable(State, -2);
				return;
			}
		}

	default:
		lua_pushnil(State);
	}
}

FLuaValue ULuaState::ToLuaValue(int Index, lua_State* State)
{
	if (!State)
		State = this->L;

	FLuaValue LuaValue;

	if (lua_isboolean(State, Index))
	{
		LuaValue.Type = ELuaValueType::Bool;
		LuaValue.Bool = lua_toboolean(State, Index) != 0;
	}
	else if (lua_type(State, Index) == LUA_TSTRING)
	{
		LuaValue.Type = ELuaValueType::String;
		LuaValue.String = FString(UTF8_TO_TCHAR(lua_tostring(State, Index)));
	}
	else if (lua_isinteger(State, Index))
	{
		LuaValue.Type = ELuaValueType::Integer;
		LuaValue.Integer = lua_tointeger(State, Index);
	}
	else if (lua_type(State, Index) == LUA_TNUMBER)
	{
		LuaValue.Type = ELuaValueType::Number;
		LuaValue.Number = lua_tonumber(State, Index);
	}
	else if (lua_istable(State, Index))
	{
		if (State != this->L)
			lua_xmove(State, this->L, 1);
		else
			lua_pushvalue(State, Index);
		LuaValue.Type = ELuaValueType::Table;
		LuaValue.LuaState = this;
		LuaValue.LuaRef = luaL_ref(this->L, LUA_REGISTRYINDEX);
	}
	else if (lua_isthread(State, Index))
	{
		if (State != this->L)
			lua_xmove(State, this->L, 1);
		else
			lua_pushvalue(State, Index);
		LuaValue.Type = ELuaValueType::Thread;
		LuaValue.LuaState = this;
		LuaValue.LuaRef = luaL_ref(this->L, LUA_REGISTRYINDEX);
	}
	else if (lua_isfunction(State, Index))
	{
		if (State != this->L)
			lua_xmove(State, this->L, 1);
		else
			lua_pushvalue(State, Index);
		LuaValue.Type = ELuaValueType::Function;
		LuaValue.LuaState = this;
		LuaValue.LuaRef = luaL_ref(this->L, LUA_REGISTRYINDEX);
	}
	else if (lua_isuserdata(State, Index))
	{
		FLuaUserData* UserData = (FLuaUserData*)lua_touserdata(State, Index);
		switch (UserData->Type)
		{
		case(ELuaValueType::UObject):
			if (UserData->Context.IsValid())
			{
				LuaValue.Type = UserData->Type;
				LuaValue.Object = UserData->Context.Get();
				LuaValue.LuaState = this;
			}
			break;
		case(ELuaValueType::UFunction):
			if (UserData->Context.IsValid() && UserData->Function.IsValid())
			{
				LuaValue.Type = UserData->Type;
				LuaValue.FunctionName = UserData->Function->GetFName();
				LuaValue.Object = UserData->Context.Get();
				LuaValue.LuaState = this;
			}
			break;
		}
	}

	return LuaValue;
}

int32 ULuaState::GetTop()
{
	return lua_gettop(L);
}

int ULuaState::MetaTableFunctionState__index(lua_State *L)
{
	ULuaState* LuaState = ULuaState::GetFromExtraSpace(L);

	FString Key = UTF8_TO_TCHAR(lua_tostring(L, 2));

	FLuaValue* LuaValue = LuaState->Table.Find(Key);
	if (LuaValue)
	{
		LuaState->FromLuaValue(*LuaValue, LuaState, L);
		return 1;
	}

	lua_pushnil(L);
	return 1;
}

int ULuaState::MetaTableFunctionState__newindex(lua_State *L)
{
	ULuaState* LuaState = ULuaState::GetFromExtraSpace(L);

	FString Key = UTF8_TO_TCHAR(lua_tostring(L, 2));

	FLuaValue* LuaValue = LuaState->Table.Find(Key);
	if (LuaValue)
	{
		*LuaValue = LuaState->ToLuaValue(3, L);
	}
	else
	{
		lua_rawset(L, 1);
	}

	return 0;
}

int ULuaState::MetaTableBlueprintFunctionLibraryState__index(lua_State *L)
{
	ULuaState* LuaState = ULuaState::GetFromExtraSpace(L);

	if (!lua_isuserdata(L, 1))
	{
		return luaL_error(L, "invalid state for BlueprintFunctionLibrary");
	}
	FLuaUserData* UserData = (FLuaUserData*)lua_touserdata(L, 1);

	if (!UserData->Context.IsValid())
	{
		return luaL_error(L, "invalid state for BlueprintFunctionLibrary");
	}

	UBlueprintFunctionLibrary* FunctionLibrary = Cast<UBlueprintFunctionLibrary>(UserData->Context);
	if (!FunctionLibrary)
		return luaL_error(L, "invalid state for BlueprintFunctionLibrary");

	FString Key = UTF8_TO_TCHAR(lua_tostring(L, 2));

	UFunction* Function = FunctionLibrary->FindFunction(FName(*Key));
	if (Function)
	{
		FLuaUserData* LuaCallContext = (FLuaUserData*)lua_newuserdata(L, sizeof(FLuaUserData));
		LuaCallContext->Type = ELuaValueType::UFunction;
		LuaCallContext->Context = FunctionLibrary;
		LuaCallContext->Function = Function;
		lua_newtable(L);
		lua_pushcfunction(L, ULuaState::MetaTableFunction__call);
		lua_setfield(L, -2, "__call");
		lua_setmetatable(L, -2);
		return 1;
	}

	lua_pushnil(L);
	return 1;
}

int ULuaState::MetaTableFunctionLuaComponent__index(lua_State *L)
{
	ULuaState* LuaState = ULuaState::GetFromExtraSpace(L);

	if (!lua_isuserdata(L, 1))
	{
		return luaL_error(L, "invalid state for ULuaComponent");
	}
	FLuaUserData* UserData = (FLuaUserData*)lua_touserdata(L, 1);

	if (!UserData->Context.IsValid())
	{
		return luaL_error(L, "invalid state for ULuaComponent");
	}

	ULuaComponent* LuaComponent = Cast<ULuaComponent>(UserData->Context.Get());
	if (!LuaComponent)
		return luaL_error(L, "invalid state for ULuaComponent");

	FString Key = UTF8_TO_TCHAR(lua_tostring(L, 2));

	FLuaValue* LuaValue = LuaComponent->Table.Find(Key);
	if (LuaValue)
	{
		LuaState->FromLuaValue(*LuaValue, LuaComponent->GetOwner(), L);
		return 1;

	}

	lua_pushnil(L);
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
	if (!UserData->Context.IsValid())
	{
		return luaL_error(L, "invalid state for ULuaComponent");
	}

	ULuaComponent* LuaComponent = Cast<ULuaComponent>(UserData->Context.Get());
	if (!LuaComponent)
		return luaL_error(L, "invalid state for ULuaComponent");

	FString Key = UTF8_TO_TCHAR(lua_tostring(L, 2));

	FLuaValue* LuaValue = LuaComponent->Table.Find(Key);
	if (LuaValue)
	{
		*LuaValue = LuaState->ToLuaValue(3, L);
	}
	else
	{
		LuaComponent->Table.Add(Key, LuaState->ToLuaValue(3, L));
	}

	return 0;
}

void ULuaState::Debug_Hook(lua_State* L, lua_Debug* ar)
{
	ULuaState* LuaState = ULuaState::GetFromExtraSpace(L);
	FLuaDebug LuaDebug;
	lua_getinfo(L, "lSn", ar);
	LuaDebug.CurrentLine = ar->currentline;
	LuaDebug.Source = UTF8_TO_TCHAR(ar->source);
	LuaDebug.Name = UTF8_TO_TCHAR(ar->name);
	LuaDebug.NameWhat = UTF8_TO_TCHAR(ar->namewhat);
	LuaDebug.What = UTF8_TO_TCHAR(ar->what);

	switch (ar->event)
	{
	case LUA_HOOKLINE:
		LuaState->ReceiveLuaLineHook(LuaDebug);
		break;
	case LUA_HOOKCALL:
		LuaState->ReceiveLuaCallHook(LuaDebug);
		break;
	case LUA_HOOKRET:
		LuaState->ReceiveLuaReturnHook(LuaDebug);
		break;
	default:
		break;
	}
}

int ULuaState::MetaTableFunctionUserData__eq(lua_State *L)
{
	ULuaState* LuaState = ULuaState::GetFromExtraSpace(L);

	if (!lua_isuserdata(L, 1))
	{
		return luaL_error(L, "invalid state for usedata");
	}

	if (!lua_isuserdata(L, 2))
	{
		lua_pushboolean(L, 0);
		return 1;
	}

	FLuaUserData* UserData = (FLuaUserData*)lua_touserdata(L, 1);
	if (!UserData->Context.IsValid())
	{
		return luaL_error(L, "invalid UObject for first userdata");
	}

	FLuaUserData* UserData2 = (FLuaUserData*)lua_touserdata(L, 2);
	if (!UserData2->Context.IsValid())
	{
		return luaL_error(L, "invalid UObject for second userdata");
	}

	if (UserData->Type == UserData2->Type && UserData->Context.Get() == UserData2->Context.Get())
	{
		if (UserData->Type == ELuaValueType::UFunction)
		{
			if (!UserData->Function.IsValid())
			{
				return luaL_error(L, "invalid UFunction for first userdata");
			}
			if (!UserData2->Function.IsValid())
			{
				return luaL_error(L, "invalid UFunction for second userdata");
			}
			if (UserData->Function.Get() == UserData2->Function.Get())
			{
				lua_pushboolean(L, 1);
				return 1;
			}
		}
		else if (UserData->Type == ELuaValueType::UObject)
		{
			lua_pushboolean(L, 1);
			return 1;
		}
	}

	lua_pushboolean(L, 0);
	return 1;
}

int ULuaState::MetaTableFunction__call(lua_State *L)
{
	ULuaState* LuaState = ULuaState::GetFromExtraSpace(L);

	if (!lua_isuserdata(L, 1))
	{
		return luaL_error(L, "invalid state for lua UFunction");
	}
	FLuaUserData* LuaCallContext = (FLuaUserData*)lua_touserdata(L, 1);

	if (!LuaCallContext->Context.IsValid() || !LuaCallContext->Function.IsValid())
	{
		return luaL_error(L, "invalid state for lua UFunction");
	}

	int NArgs = lua_gettop(L);

	FScopeCycleCounterUObject ObjectScope(LuaCallContext->Context.Get());
	FScopeCycleCounterUObject FunctionScope(LuaCallContext->Function.Get());

	void* Parameters = FMemory_Alloca(LuaCallContext->Function->ParmsSize);
	FMemory::Memzero(Parameters, LuaCallContext->Function->ParmsSize);

	int StackPointer = 2;
	// arguments
	for (TFieldIterator<UProperty> FArgs(LuaCallContext->Function.Get()); FArgs && ((FArgs->PropertyFlags & (CPF_Parm | CPF_ReturnParm)) == CPF_Parm); ++FArgs)
	{
		UProperty *Prop = *FArgs;
		UStructProperty* LuaProp = Cast<UStructProperty>(Prop);
		if (!LuaProp)
		{
			UArrayProperty* ArrayProp = Cast<UArrayProperty>(Prop);
			if (ArrayProp)
			{
				LuaProp = Cast<UStructProperty>(ArrayProp->Inner);
				if (!LuaProp)
					break;
				if (LuaProp->Struct != FLuaValue::StaticStruct())
					break;
				// start filling the array with the rest of arguments
				int ArgsToProcess = NArgs - StackPointer + 1;
				if (ArgsToProcess < 1)
					break;
				FScriptArrayHelper_InContainer ArrayHelper(ArrayProp, LuaProp->ContainerPtrToValuePtr<uint8>(Parameters));
				ArrayHelper.AddValues(ArgsToProcess);
				for (int i = StackPointer; i < StackPointer + ArgsToProcess; i++)
				{
					FLuaValue LuaValue = LuaState->ToLuaValue(i, L);
					*LuaProp->ContainerPtrToValuePtr<FLuaValue>(ArrayHelper.GetRawPtr(i - StackPointer)) = LuaValue;
				}
			}
			break;
		}
		if (LuaProp->Struct != FLuaValue::StaticStruct())
			break;

		FLuaValue LuaValue = LuaState->ToLuaValue(StackPointer++, L);
		*LuaProp->ContainerPtrToValuePtr<FLuaValue>(Parameters) = LuaValue;
	}

	LuaState->InceptionLevel++;
	LuaCallContext->Context->ProcessEvent(LuaCallContext->Function.Get(), Parameters);
	check(LuaState->InceptionLevel > 0);
	LuaState->InceptionLevel--;

	if (LuaState->InceptionLevel == 0)
	{
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
	}

	int ReturnedValues = 0;

	// get return value
	for (TFieldIterator<UProperty> FArgs(LuaCallContext->Function.Get()); FArgs; ++FArgs)
	{
		UProperty *Prop = *FArgs;
		if (!Prop->HasAnyPropertyFlags(CPF_ReturnParm | CPF_OutParm))
			continue;

		// avoid input args (at all costs !)
		if (Prop->HasAnyPropertyFlags(CPF_ConstParm | CPF_ReferenceParm))
			continue;

		UStructProperty* LuaProp = Cast<UStructProperty>(Prop);
		if (!LuaProp)
		{
			UArrayProperty* ArrayProp = Cast<UArrayProperty>(Prop);
			if (ArrayProp)
			{
				LuaProp = Cast<UStructProperty>(ArrayProp->Inner);
				if (!LuaProp)
					break;
				if (LuaProp->Struct != FLuaValue::StaticStruct())
					break;

				FScriptArrayHelper_InContainer ArrayHelper(ArrayProp, LuaProp->ContainerPtrToValuePtr<uint8>(Parameters));
				for (int i = 0; i < ArrayHelper.Num(); i++)
				{
					FLuaValue* LuaValue = LuaProp->ContainerPtrToValuePtr<FLuaValue>(ArrayHelper.GetRawPtr(i));
					ReturnedValues++;
					LuaState->FromLuaValue(*LuaValue, nullptr, L);
				}


			}
			break;
		}

		if (LuaProp->Struct != FLuaValue::StaticStruct())
			break;

		FLuaValue* LuaValue = LuaProp->ContainerPtrToValuePtr<FLuaValue>(Parameters);
		if (LuaValue)
		{
			ReturnedValues++;
			LuaState->FromLuaValue(*LuaValue, nullptr, L);
		}
	}

	if (ReturnedValues > 0)
		return ReturnedValues;

	lua_pushnil(L);
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
		lua_call(L, 1, 1);
		const char *s = lua_tostring(L, -1);
		if (!s)
			return luaL_error(L, "'tostring must return a string to 'print'");
		FString Value = FString(UTF8_TO_TCHAR(s));
		lua_pop(L, 1);
		Messages.Add(Value);
	}
	LuaState->Log(FString::Join(Messages, TEXT("\t")));
	return 0;
}

int ULuaState::TableFunction_package_preload(lua_State *L)
{
	ULuaState* LuaState = ULuaState::GetFromExtraSpace(L);

	FString Key = UTF8_TO_TCHAR(lua_tostring(L, 1));

	if (LuaState->L != L)
		return luaL_error(L, "you cannot call package.preload from a thread/coroutine (error while loading %s)", TCHAR_TO_UTF8(*Key));

	// first check for BlueprintFunctionLibrary
	TSubclassOf<UBlueprintFunctionLibrary>* FunctionLibrary = LuaState->RequireBlueprintFunctionLibraryTable.Find(Key);
	if (FunctionLibrary)
	{
		LuaState->NewUObject(FunctionLibrary->GetDefaultObject());
		// metatable
		LuaState->NewTable();
		LuaState->PushCFunction(MetaTableBlueprintFunctionLibraryState__index);
		LuaState->SetField(-2, "__index");
		LuaState->SetMetaTable(-2);
		return 1;
	}
	// then check for code assets
	ULuaCode** LuaCodePtr = LuaState->RequireTable.Find(Key);
	if (!LuaCodePtr)
	{
		if (LuaState->bAddProjectContentDirToPackagePath && LuaState->RunFile(Key + ".lua", true, 1))
		{
			return 1;
		}
		return luaL_error(L, "%s", lua_tostring(L, -1));

		// now search in additional paths
		for (FString AdditionalPath : LuaState->AppendProjectContentDirSubDir)
		{
			if (LuaState->RunFile(AdditionalPath / Key + ".lua", true, 1))
			{
				return 1;
			}
			return luaL_error(L, "%s", lua_tostring(L, -1));

		}
		return luaL_error(L, "unable to find package %s", TCHAR_TO_UTF8(*Key));
	}

	ULuaCode* LuaCode = *LuaCodePtr;
	if (!LuaCode)
	{
		return luaL_error(L, "LuaCodeAsset not set for package %s", TCHAR_TO_UTF8(*Key));
	}

	if (!LuaState->RunCodeAsset(LuaCode, 1))
	{
		return luaL_error(L, "%s", lua_tostring(L, -1));
	}

	return 1;
}

void ULuaState::ReceiveLuaError_Implementation(const FString& Message)
{

}

void ULuaState::ReceiveLuaCallHook_Implementation(const FLuaDebug& LuaDebug)
{

}

void ULuaState::ReceiveLuaReturnHook_Implementation(const FLuaDebug& LuaDebug)
{

}

void ULuaState::ReceiveLuaLineHook_Implementation(const FLuaDebug& LuaDebug)
{

}

void ULuaState::NewTable()
{
	lua_newtable(L);
}

void ULuaState::SetMetaTable(int Index)
{
	lua_setmetatable(L, Index);
}

void ULuaState::GetMetaTable(int Index)
{
	lua_getmetatable(L, Index);
}

void ULuaState::SetField(int Index, const char* FieldName)
{
	lua_setfield(L, Index, FieldName);
}

void ULuaState::GetField(int Index, const char* FieldName)
{
	lua_getfield(L, Index, FieldName);
}

void ULuaState::RawGetI(int Index, int N)
{
	lua_rawgeti(L, Index, N);
}

void ULuaState::RawSetI(int Index, int N)
{
	lua_rawseti(L, Index, N);
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
			LastError = FString::Printf(TEXT("Lua key \"%s\" is nil"), *Parts[i]);
			if (bLogError)
				LogError(LastError);
			ReceiveLuaError(LastError);
			return i + 1 + AdditionalPop;
		}
	}
	return i + AdditionalPop;
}

void ULuaState::SetFieldFromTree(FString Tree, FLuaValue& Value, bool bGlobal)
{
	TArray<FString> Parts;
	Tree.ParseIntoArray(Parts, TEXT("."));

	int32 ItemsToPop = GetFieldFromTree(Tree, bGlobal);
	// invalid key
	if (ItemsToPop != (Parts.Num() + (bGlobal ? 1 : 0)))
	{
		Pop(ItemsToPop);
		return;
	}

	Pop();
	FromLuaValue(Value);
	SetField(-2, TCHAR_TO_UTF8(*Parts.Last()));
	Pop(ItemsToPop - 1);
}


void ULuaState::NewUObject(UObject* Object)
{
	FLuaUserData* UserData = (FLuaUserData*)lua_newuserdata(L, sizeof(FLuaUserData));
	UserData->Type = ELuaValueType::UObject;
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

int ULuaState::NewRef()
{
	return luaL_ref(L, LUA_REGISTRYINDEX);
}

void ULuaState::GetRef(int Ref)
{
	lua_rawgeti(L, LUA_REGISTRYINDEX, Ref);
}

int ULuaState::Next(int Index)
{
	return lua_next(L, Index);
}

bool ULuaState::Resume(int Index, int NArgs)
{
	lua_State* Coroutine = lua_tothread(L, Index);
	if (!Coroutine)
		return false;

	if (lua_status(Coroutine) == LUA_OK && lua_gettop(Coroutine) == 0)
	{
		lua_pushboolean(L, 0);
		lua_pushstring(L, "Lua error: cannot resume dead coroutine");
		return false;
	}

	lua_xmove(L, Coroutine, NArgs);
	int Ret = lua_resume(Coroutine, L, NArgs);
	if (Ret != LUA_OK && Ret != LUA_YIELD)
	{
		lua_pushboolean(L, 0);
		lua_xmove(Coroutine, L, 1);
		return false;
	}

	int NRet = lua_gettop(Coroutine);
	lua_pushboolean(L, 1);
	lua_xmove(Coroutine, L, NRet);
	return true;
}

int ULuaState::GC(int What, int Data)
{
	return lua_gc(L, What, Data);
}

void ULuaState::Len(int Index)
{
	lua_len(L, Index);
}

int32 ULuaState::ToInteger(int Index)
{
	return lua_tointeger(L, Index);
}

FLuaValue ULuaState::CreateLuaTable()
{
	FLuaValue NewTable;
	NewTable.Type = ELuaValueType::Table;
	NewTable.LuaState = this;
	return NewTable;
}

ULuaState::~ULuaState()
{
	if (L)
		lua_close(L);

	FLuaMachineModule::Get().UnregisterLuaState(this);
}


#define LUAVALUE_PROP_CAST(Type, Type2) Type* __##Type##__ = Cast<Type>(Property);\
	if (__##Type##__)\
	{\
		return FLuaValue((Type2)__##Type##__->GetPropertyValue_InContainer(Buffer, Index));\
	}

#define LUAVALUE_PROP_CAST_TOSTRING(Type) Type* __##Type##__ = Cast<Type>(Property);\
	if (__##Type##__)\
	{\
		return FLuaValue(__##Type##__->GetPropertyValue_InContainer(Buffer, Index).ToString());\
	}

#define LUAVALUE_PROP_SET(Type, Value) Type* __##Type##__ = Cast<Type>(Property);\
	if (__##Type##__)\
	{\
		__##Type##__->SetPropertyValue_InContainer(Buffer, Value, Index);\
		return;\
	}

FLuaValue ULuaState::FromUProperty(void* Buffer, UProperty* Property, bool& bSuccess, int32 Index)
{
	bSuccess = true;

	LUAVALUE_PROP_CAST(UBoolProperty, bool);
	LUAVALUE_PROP_CAST(UFloatProperty, float);
	LUAVALUE_PROP_CAST(UIntProperty, int32);
	LUAVALUE_PROP_CAST(UUInt32Property, int32);
	LUAVALUE_PROP_CAST(UInt16Property, int32);
	LUAVALUE_PROP_CAST(UInt8Property, int32);
	LUAVALUE_PROP_CAST(UByteProperty, int32);
	LUAVALUE_PROP_CAST(UUInt16Property, int32);

	LUAVALUE_PROP_CAST(UStrProperty, FString);
	LUAVALUE_PROP_CAST_TOSTRING(UNameProperty);
	LUAVALUE_PROP_CAST_TOSTRING(UTextProperty);

	LUAVALUE_PROP_CAST(UClassProperty, UObject*);
	LUAVALUE_PROP_CAST(UObjectProperty, UObject*);


	UObjectPropertyBase* ObjectPropertyBase = Cast<UObjectPropertyBase>(Property);
	if (ObjectPropertyBase)
	{
		return FLuaValue(ObjectPropertyBase->GetObjectPropertyValue_InContainer(Buffer, Index));
	}

	UWeakObjectProperty* WeakObjectProperty = Cast<UWeakObjectProperty>(Property);
	if (WeakObjectProperty)
	{
		const FWeakObjectPtr& WeakPtr = WeakObjectProperty->GetPropertyValue_InContainer(Buffer, Index);
		return FLuaValue(WeakPtr.Get());
	}

	bSuccess = false;
	return FLuaValue();
}

void ULuaState::ToUProperty(void* Buffer, UProperty* Property, FLuaValue Value, bool& bSuccess, int32 Index)
{
	bSuccess = true;

	LUAVALUE_PROP_SET(UBoolProperty, Value.ToBool());
	LUAVALUE_PROP_SET(UFloatProperty, Value.ToFloat());
	LUAVALUE_PROP_SET(UIntProperty, Value.ToInteger());
	LUAVALUE_PROP_SET(UUInt32Property, Value.ToInteger());
	LUAVALUE_PROP_SET(UInt16Property, Value.ToInteger());
	LUAVALUE_PROP_SET(UInt8Property, Value.ToInteger());
	LUAVALUE_PROP_SET(UByteProperty, Value.ToInteger());
	LUAVALUE_PROP_SET(UUInt16Property, Value.ToInteger());

	LUAVALUE_PROP_SET(UStrProperty, Value.ToString());
	LUAVALUE_PROP_SET(UNameProperty, Value.ToName());
	LUAVALUE_PROP_SET(UTextProperty, FText::FromString(Value.ToString()));

	LUAVALUE_PROP_SET(UClassProperty, Value.Object);
	LUAVALUE_PROP_SET(UObjectProperty, Value.Object);

	UObjectPropertyBase* ObjectPropertyBase = Cast<UObjectPropertyBase>(Property);
	if (ObjectPropertyBase)
	{
		ObjectPropertyBase->SetObjectPropertyValue_InContainer(Buffer, Value.Object, Index);
	}

	UWeakObjectProperty* WeakObjectProperty = Cast<UWeakObjectProperty>(Property);
	if (WeakObjectProperty)
	{
		FWeakObjectPtr WeakPtr(Value.Object);
		WeakObjectProperty->SetPropertyValue_InContainer(Buffer, WeakPtr, Index);
		return;
	}

	bSuccess = false;
}

void ULuaState::SetUserDataMetaTable(FLuaValue MetaTable)
{
	UserDataMetaTable = MetaTable;
}

