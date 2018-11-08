// Copyright 2018 - Roberto De Ioris

#include "LuaState.h"
#include "LuaComponent.h"
#include "LuaMachine.h"
#include "GameFramework/Actor.h"
#include "Runtime/Core/Public/Misc/FileHelper.h"
#include "Runtime/Core/Public/Misc/Paths.h"
#include "Runtime/Core/Public/Serialization/BufferArchive.h"

ULuaState::ULuaState()
{
	L = nullptr;
	bLuaOpenLibs = true;
	bDisabled = false;
	bLogError = true;
	bAddProjectContentDirToPackagePath = true;
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
		bool bHasError = false;
		if (!RunFile(LuaFilename, true, bHasError))
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

	return this;
}

bool ULuaState::RunCodeAsset(ULuaCode* CodeAsset, int NRet)
{

	if (CodeAsset->bCooked && CodeAsset->bCookAsBytecode)
	{
		return RunCode(CodeAsset->ByteCode, CodeAsset->GetPathName(), NRet);
	}

	return RunCode(CodeAsset->Code.ToString(), CodeAsset->GetPathName(), NRet);

}

bool ULuaState::RunFile(FString Filename, bool bIgnoreNonExistent, bool& bHasError, int NRet)
{
	TArray<uint8> Code;
	FString AbsoluteFilename = FPaths::Combine(FPaths::ProjectContentDir(), Filename);

	if (!FPaths::FileExists(AbsoluteFilename))
	{
		if (bIgnoreNonExistent)
			return true;
		LastError = FString::Printf(TEXT("Unable to open file %s"), *Filename);
		return false;
	}

	if (FFileHelper::LoadFileToArray(Code, *AbsoluteFilename))
	{
		if (RunCode(Code, AbsoluteFilename, NRet))
		{
			return true;
		}
		bHasError = true;
		return false;
	}

	bHasError = true;
	LastError = FString::Printf(TEXT("Unable to open file %s"), *Filename);
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

void ULuaState::FromLuaValue(FLuaValue& LuaValue, UObject* CallContext)
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
		if (LuaValue.LuaRef == LUA_NOREF)
		{
			lua_newtable(L);
			lua_pushvalue(L, -1);
			LuaValue.LuaRef = luaL_ref(L, LUA_REGISTRYINDEX);
			LuaValue.LuaState = this;
			break;
		}
		if (this != LuaValue.LuaState)
		{
			lua_pushnil(L);
			break;
		}
		lua_rawgeti(L, LUA_REGISTRYINDEX, LuaValue.LuaRef);
		break;
	case ELuaValueType::Function:
		if (this != LuaValue.LuaState)
		{
			lua_pushnil(L);
			break;
		}
		lua_rawgeti(L, LUA_REGISTRYINDEX, LuaValue.LuaRef);
		break;
	case ELuaValueType::UObject:
	{
		ULuaComponent* LuaComponent = Cast<ULuaComponent>(LuaValue.Object);
		if (LuaComponent)
		{
			NewUObject(LuaComponent);
			if (LuaComponent->LuaState == GetClass())
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
					LuaComponent = Cast<ULuaComponent>(Component);
					if (LuaComponent->LuaState == GetClass())
					{
						NewUObject(LuaComponent);
						LuaComponent->SetupMetatable();
						return;
					}
				}
			}
		}
		NewUObject(LuaValue.Object);
		break;
	}
	case ELuaValueType::UFunction:
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
				FLuaUserData* LuaCallContext = (FLuaUserData*)lua_newuserdata(L, sizeof(FLuaUserData));
				LuaCallContext->Type = ELuaValueType::UFunction;
				LuaCallContext->Context = CallContext;
				LuaCallContext->Function = Function;
				NewTable();
				PushCFunction(ULuaState::MetaTableFunction__call);
				SetField(-2, "__call");
				SetMetaTable(-2);
				return;
			}
		}

	default:
		lua_pushnil(L);
	}
}

FLuaValue ULuaState::ToLuaValue(int Index)
{
	FLuaValue LuaValue;
	LuaValue.LuaState = this;

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
	else if (lua_istable(L, Index))
	{
		lua_pushvalue(L, Index);
		LuaValue.Type = ELuaValueType::Table;
		LuaValue.LuaRef = luaL_ref(L, LUA_REGISTRYINDEX);
	}
	else if (lua_isfunction(L, Index))
	{
		lua_pushvalue(L, Index);
		LuaValue.Type = ELuaValueType::Function;
		LuaValue.LuaRef = luaL_ref(L, LUA_REGISTRYINDEX);
	}
	else if (lua_isuserdata(L, Index))
	{
		FLuaUserData* UserData = (FLuaUserData*)lua_touserdata(L, Index);
		switch (UserData->Type)
		{
		case(ELuaValueType::UObject):
			if (UserData->Context.IsValid())
			{
				LuaValue.Type = UserData->Type;
				LuaValue.Object = UserData->Context.Get();
			}
			break;
		case(ELuaValueType::UFunction):
			if (UserData->Context.IsValid() && UserData->Function.IsValid())
			{
				LuaValue.Type = UserData->Type;
				LuaValue.FunctionName = UserData->Function->GetFName();
				LuaValue.Object = UserData->Context.Get();
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
		LuaState->FromLuaValue(*LuaValue, LuaState);
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
		*LuaValue = LuaState->ToLuaValue(3);
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
		LuaState->FromLuaValue(*LuaValue, LuaComponent->GetOwner());
		return 1;

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
		*LuaValue = LuaState->ToLuaValue(3);
	}
	else
	{
		LuaComponent->Table.Add(Key, LuaState->ToLuaValue(3));
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

	if (!LuaCallContext->Context.IsValid() || !LuaCallContext->Function.IsValid())
	{
		return luaL_error(L, "invalid state for lua UFunction");
	}

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
			continue;
		if (LuaProp->Struct != FLuaValue::StaticStruct())
			continue;

		FLuaValue LuaValue = LuaState->ToLuaValue(StackPointer++);
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


	// get return value
	for (TFieldIterator<UProperty> FArgs(LuaCallContext->Function.Get()); FArgs; ++FArgs)
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

	// first check for code assets
	ULuaCode** LuaCodePtr = LuaState->RequireTable.Find(Key);
	if (!LuaCodePtr)
	{
		bool bLuaError = false;
		if (LuaState->bAddProjectContentDirToPackagePath && LuaState->RunFile(Key + ".lua", true, bLuaError, 1))
		{
			return 1;
		}
		if (bLuaError)
		{
			return luaL_error(L, "%s", lua_tostring(L, -1));
		}
		// now search in additional paths
		for (FString AdditionalPath : LuaState->AppendProjectContentDirSubDir)
		{
			if (LuaState->RunFile(AdditionalPath / Key + ".lua", true, bLuaError, 1))
			{
				return 1;
			}
			if (bLuaError)
			{
				return luaL_error(L, "%s", lua_tostring(L, -1));
			}
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
			LastError = FString::Printf(TEXT("unknown Lua key: \"%s\""), *Parts[i]);
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
	int32 ItemsToPop = GetFieldFromTree(Tree, bGlobal);
	// invalid key
	if (ItemsToPop == 1 && bGlobal)
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

