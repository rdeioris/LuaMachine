// Copyright 2018-2023 - Roberto De Ioris

#include "LuaState.h"
#include "LuaComponent.h"
#include "LuaUserDataObject.h"
#include "LuaMachine.h"
#include "LuaBlueprintPackage.h"
#include "LuaBlueprintFunctionLibrary.h"
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION > 0
#include "AssetRegistry/AssetRegistryModule.h"
#else
#include "AssetRegistryModule.h"
#endif
#include "GameFramework/Actor.h"
#include "Runtime/Core/Public/Misc/FileHelper.h"
#include "Runtime/Core/Public/Misc/Paths.h"
#include "Runtime/Core/Public/Serialization/BufferArchive.h"
#include "Runtime/CoreUObject/Public/UObject/TextProperty.h"

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
	bEnableCountHook = false;
	bRawLuaFunctionCall = false;

	FCoreUObjectDelegates::GetPostGarbageCollect().AddUObject(this, &ULuaState::GCLuaDelegatesCheck);
}

ULuaState* ULuaState::GetLuaState(UWorld* InWorld)
{
	CurrentWorld = InWorld;

	if (L != nullptr)
	{
		return this;
	}

	if (bDisabled)
	{
		return nullptr;
	}

	L = luaL_newstate();

	if (bLuaOpenLibs)
	{
		luaL_openlibs(L);
	}

	// load "package" for allowing minimal setup
	luaL_requiref(L, "package", luaopen_package, 1);
	lua_pop(L, 1);

	if (!bLuaOpenLibs)
	{
		if (LuaLibsLoader.bLoadBase)
		{
			luaL_requiref(L, "_G", luaopen_base, 1);
			lua_pop(L, 1);
		}

		if (LuaLibsLoader.bLoadCoroutine)
		{
			luaL_requiref(L, "coroutine", luaopen_coroutine, 1);
			lua_pop(L, 1);
		}

		if (LuaLibsLoader.bLoadTable)
		{
			luaL_requiref(L, "table", luaopen_table, 1);
			lua_pop(L, 1);
		}

		if (LuaLibsLoader.bLoadIO)
		{
			luaL_requiref(L, "io", luaopen_io, 1);
			lua_pop(L, 1);
		}

		if (LuaLibsLoader.bLoadOS)
		{
			luaL_requiref(L, "os", luaopen_os, 1);
			lua_pop(L, 1);
		}

		if (LuaLibsLoader.bLoadString)
		{
			luaL_requiref(L, "string", luaopen_string, 1);
			lua_pop(L, 1);
		}

		if (LuaLibsLoader.bLoadMath)
		{
			luaL_requiref(L, "math", luaopen_math, 1);
			lua_pop(L, 1);
		}

		if (LuaLibsLoader.bLoadUTF8)
		{
			luaL_requiref(L, "utf8", luaopen_utf8, 1);
			lua_pop(L, 1);
		}

		if (LuaLibsLoader.bLoadDebug)
		{
			luaL_requiref(L, "debug", luaopen_debug, 1);
			lua_pop(L, 1);
		}
	}

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
		OverridePackagePath.ReplaceInline(*FString("$(CONTENT_DIR)"), *FPaths::ProjectContentDir());
		lua_pushstring(L, TCHAR_TO_ANSI(*OverridePackagePath));
		SetField(-2, "path");
	}

	if (bAddProjectContentDirToPackagePath)
	{
		GetField(-1, "path");
		const char* CurrentLuaPath = lua_tostring(L, -1);
		FString NewPackagePath = FString(CurrentLuaPath) + ";" + FPaths::ProjectContentDir() + "/?.lua";
		Pop();
		lua_pushstring(L, TCHAR_TO_ANSI(*NewPackagePath));
		SetField(-2, "path");
	}

	for (FString SubDir : AppendProjectContentDirSubDir)
	{
		GetField(-1, "path");
		const char* CurrentLuaPath = lua_tostring(L, -1);
		FString NewPackagePath = FString(CurrentLuaPath) + ";" + FPaths::ProjectContentDir() / SubDir + "/?.lua";
		Pop();
		lua_pushstring(L, TCHAR_TO_ANSI(*NewPackagePath));
		SetField(-2, "path");
	}

	if (!OverridePackageCPath.IsEmpty())
	{
		OverridePackageCPath.ReplaceInline(*FString("$(CONTENT_DIR)"), *FPaths::ProjectContentDir());

		static const FString libExtension =
#if PLATFORM_MAC || PLATFORM_IOS
			FString("dylib");
#elif PLATFORM_LINUX || PLATFORM_ANDROID
			FString("so");
#elif PLATFORM_WINDOWS
			FString("dll");
#else
			FString("");
#endif

		OverridePackageCPath.ReplaceInline(*FString("$(LIB_EXT)"), *libExtension);

		lua_pushstring(L, TCHAR_TO_ANSI(*OverridePackageCPath));
		SetField(-2, "cpath");
	}
	// manage RequireTable
	GetField(-1, "preload");
	for (TPair<FString, ULuaCode*>& Pair : RequireTable)
	{
		PushCFunction(ULuaState::TableFunction_package_preload);
		SetField(-2, TCHAR_TO_ANSI(*Pair.Key));
	}

	// pop package.prelod
	Pop(1);

	// manage searchers
	GetField(-1, "searchers");
	PushCFunction(ULuaState::TableFunction_package_loader);
	constexpr int PackageLoadersFirstAvailableIndex = 5;
	lua_seti(L, -2, PackageLoadersFirstAvailableIndex);

	// pop package.searchers (and package)
	Pop(2);


	for (TPair<FString, FLuaValue>& Pair : Table)
	{
		FromLuaValue(Pair.Value, this, L);
		SetField(-2, TCHAR_TO_ANSI(*Pair.Key));
	}

	for (TPair<FString, TSubclassOf<ULuaBlueprintPackage>>& Pair : LuaBlueprintPackagesTable)
	{
		if (Pair.Value)
		{
			NewTable();
			ULuaBlueprintPackage* LuaBlueprintPackage = NewObject<ULuaBlueprintPackage>(this, Pair.Value);
			if (LuaBlueprintPackage)
			{
				for (auto LuaPair : LuaBlueprintPackage->Table)
				{
					FromLuaValue(LuaPair.Value, LuaBlueprintPackage);
					SetField(-2, TCHAR_TO_ANSI(*LuaPair.Key));
				}
				// this avoid the package to be GC'd
				LuaBlueprintPackages.Add(Pair.Key, LuaBlueprintPackage);
				LuaBlueprintPackage->SelfTable = ToLuaValue(-1);
				LuaBlueprintPackage->Init();
				LuaBlueprintPackage->ReceiveInit();
			}
		}
		else
		{
			PushNil();
		}
		SetField(-2, TCHAR_TO_ANSI(*Pair.Key));
	}

	// pop global table
	Pop();

	// This allows subclasses to do any last minute initialization on lua state before
	// we load code
	ReceiveLuaStatePreInitialized();

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
	if (bEnableCountHook)
	{
		DebugMask |= LUA_MASKCOUNT;
	}

	if (DebugMask != 0)
	{
		lua_sethook(L, Debug_Hook, DebugMask, HookInstructionCount);
	}

	if (LuaCodeAsset)
	{
		if (!RunCodeAsset(LuaCodeAsset))
		{
			if (bLogError)
				LogError(LastError);
			ReceiveLuaError(LastError);
			bDisabled = true;
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
			return nullptr;
		}
		UserDataMetaTable = ToLuaValue(-1);
		Pop();
	}

	LuaStateInit();
	ReceiveLuaStateInitialized();

#if WITH_EDITOR
	if (!(GetFlags() & RF_ClassDefaultObject))
	{
		LuaConsole.LuaState = this;
		IModularFeatures::Get().RegisterModularFeature(IConsoleCommandExecutor::ModularFeatureName(), &LuaConsole);
	}
#endif


	return this;
}

FLuaValue ULuaState::GetLuaBlueprintPackageTable(const FString& PackageName)
{
	if (!LuaBlueprintPackages.Contains(PackageName))
	{
		return FLuaValue();
	}

	return LuaBlueprintPackages[PackageName]->SelfTable;
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

bool ULuaState::RunFile(const FString& Filename, bool bIgnoreNonExistent, int NRet, bool bNonContentDirectory)
{
	TArray<uint8> Code;
	FString AbsoluteFilename = FPaths::Combine(FPaths::ProjectContentDir(), Filename);

	if (bNonContentDirectory)
	{
		AbsoluteFilename = Filename;
	}

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

bool ULuaState::RunCode(const FString& Code, const FString& CodePath, int NRet)
{
	TArray<uint8> Bytes;
	Bytes.Append((uint8*)TCHAR_TO_UTF8(*Code), FCStringAnsi::Strlen(TCHAR_TO_UTF8(*Code)));
	return RunCode(Bytes, CodePath, NRet);
}

bool ULuaState::RunCode(const TArray<uint8>& Code, const FString& CodePath, int NRet)
{
	FString FullCodePath = FString("@") + CodePath;

	if (luaL_loadbuffer(L, (const char*)Code.GetData(), Code.Num(), TCHAR_TO_ANSI(*FullCodePath)))
	{
		LastError = FString::Printf(TEXT("Lua loading error: %s"), ANSI_TO_TCHAR(lua_tostring(L, -1)));
		return false;
	}
	else
	{

		if (lua_pcall(L, 0, NRet, 0))
		{
			LastError = FString::Printf(TEXT("Lua execution error: %s"), ANSI_TO_TCHAR(lua_tostring(L, -1)));
			return false;
		}
	}

	return true;
}

int ULuaState::ToByteCode_Writer(lua_State* L, const void* Ptr, size_t Size, void* UserData)
{
	TArray<uint8>* Output = (TArray<uint8>*)UserData;
	Output->Append((uint8*)Ptr, Size);
	return 0;
}

TArray<uint8> ULuaState::ToByteCode(const FString& Code, const FString& CodePath, FString& ErrorString)
{
	const TCHAR* CodeRaw = *Code;
	FString FullCodePath = FString("@") + CodePath;
	TArray<uint8> Output;

	lua_State* L = luaL_newstate();
	if (luaL_loadbuffer(L, TCHAR_TO_UTF8(CodeRaw), FCStringAnsi::Strlen(TCHAR_TO_UTF8(CodeRaw)), TCHAR_TO_ANSI(*FullCodePath)))
	{
		ErrorString = ANSI_TO_TCHAR(lua_tostring(L, -1));
		Output.Empty();
		lua_close(L);
		return Output;
	}

	if (lua_dump(L, ULuaState::ToByteCode_Writer, &Output, 1))
	{
		ErrorString = ANSI_TO_TCHAR(lua_tostring(L, -1));
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
	{
		State = this->L;
	}

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
	{
		TArray<uint8> Bytes = LuaValue.ToBytes();
		lua_pushlstring(State, (const char*)Bytes.GetData(), Bytes.Num());
	}
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
		if (!LuaValue.Object)
		{
			lua_pushnil(State);
			break;
		}

		NewUObject(LuaValue.Object, State);
		if (ULuaComponent* LuaComponent = Cast<ULuaComponent>(LuaValue.Object))
		{
			if (!LuaComponent->LuaState)
			{
				UE_LOG(LogLuaMachine, Warning, TEXT("%s has no associated LuaState"), *LuaComponent->GetFullName());
			}
			// ensure we are in the same LuaState
			else if (LuaComponent->LuaState == GetClass())
			{
				SetupAndAssignUserDataMetatable(LuaComponent, LuaComponent->Metatable, State);
			}
		}
		else if (ULuaUserDataObject* LuaUserDataObject = Cast<ULuaUserDataObject>(LuaValue.Object))
		{
			if (!LuaUserDataObject->GetLuaState())
			{
				UE_LOG(LogLuaMachine, Warning, TEXT("%s has no associated LuaState"), *LuaUserDataObject->GetFullName());
			}
			// ensure we are in the same LuaState
			else if (LuaUserDataObject->GetLuaState() == GetClass())
			{
				SetupAndAssignUserDataMetatable(LuaUserDataObject, LuaUserDataObject->Metatable, State);
			}
		}
		else
		{
			if (UserDataMetaTable.Type == ELuaValueType::Table)
			{
				FromLuaValue(UserDataMetaTable, nullptr, State);
			}
			else
			{
				lua_newtable(State);
				// allow comparison between userdata/UObject/UFunction
				lua_pushcfunction(State, ULuaState::MetaTableFunctionUserData__eq);
				lua_setfield(State, -2, "__eq");
			}
			lua_setmetatable(State, -2);
		}
	}
	break;
	case ELuaValueType::UFunction:
		// if no context is assigned to the function, own it !
		if (!LuaValue.LuaState.IsValid())
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
			UObject* FunctionOwner = CallContext;
			if (ULuaComponent* LuaComponent = Cast<ULuaComponent>(CallContext))
			{
				FunctionOwner = LuaComponent->GetOwner();
			}

			if (FunctionOwner)
			{
				UFunction* Function = FunctionOwner->FindFunction(LuaValue.FunctionName);
				if (Function)
				{
					// cache it for context-less calls
					LuaValue.Object = CallContext;
					FLuaUserData* LuaCallContext = (FLuaUserData*)lua_newuserdata(State, sizeof(FLuaUserData));
					LuaCallContext->Type = ELuaValueType::UFunction;
					LuaCallContext->Context = CallContext;
					LuaCallContext->Function = Function;
					lua_newtable(State);
					lua_pushcfunction(State, bRawLuaFunctionCall ? ULuaState::MetaTableFunction__rawcall : ULuaState::MetaTableFunction__call);
					lua_setfield(State, -2, "__call");
					lua_setmetatable(State, -2);
					return;
				}
			}
		}
		// no function found
		lua_pushnil(State);
		break;
	case ELuaValueType::MulticastDelegate:
		// if no context is assigned to the function, own it !
		if (!LuaValue.LuaState.IsValid())
		{
			LuaValue.LuaState = this;
		}

		if (this != LuaValue.LuaState)
		{
			lua_pushnil(State);
			break;
		}
		{
			FLuaUserData* LuaCallContext = (FLuaUserData*)lua_newuserdata(State, sizeof(FLuaUserData));
			LuaCallContext->Type = ELuaValueType::MulticastDelegate;
			LuaCallContext->Function = reinterpret_cast<UFunction*>(LuaValue.Object);
			LuaCallContext->MulticastScriptDelegate = LuaValue.MulticastScriptDelegate;
			lua_newtable(State);
			lua_pushcfunction(State, bRawLuaFunctionCall ? ULuaState::MetaTableFunction__rawbroadcast : ULuaState::MetaTableFunction__rawbroadcast);
			lua_setfield(State, -2, "__call");
			lua_setmetatable(State, -2);
			return;
		}
		break;
	default:
		lua_pushnil(State);
	}
}

FLuaValue ULuaState::ToLuaValue(int Index, lua_State* State)
{
	if (!State)
	{
		State = this->L;
	}

	FLuaValue LuaValue;

	if (lua_isboolean(State, Index))
	{
		LuaValue = FLuaValue(lua_toboolean(State, Index) != 0);
	}
	else if (lua_type(State, Index) == LUA_TSTRING)
	{
		size_t StringLength = 0;
		const char* String = lua_tolstring(State, Index, &StringLength);
		LuaValue = FLuaValue(String, StringLength);
	}
	else if (lua_isinteger(State, Index))
	{
		LuaValue = FLuaValue(lua_tointeger(State, Index));
	}
	else if (lua_type(State, Index) == LUA_TNUMBER)
	{
		LuaValue = FLuaValue(lua_tonumber(State, Index));
	}
	else if (lua_istable(State, Index))
	{
		lua_pushvalue(State, Index);
		if (State != this->L)
			lua_xmove(State, this->L, 1);
		LuaValue.Type = ELuaValueType::Table;
		LuaValue.LuaState = this;
		LuaValue.LuaRef = luaL_ref(this->L, LUA_REGISTRYINDEX);
	}
	else if (lua_isthread(State, Index))
	{
		lua_pushvalue(State, Index);
		if (State != this->L)
			lua_xmove(State, this->L, 1);
		LuaValue.Type = ELuaValueType::Thread;
		LuaValue.LuaState = this;
		LuaValue.LuaRef = luaL_ref(this->L, LUA_REGISTRYINDEX);
	}
	else if (lua_isfunction(State, Index))
	{
		lua_pushvalue(State, Index);
		if (State != this->L)
			lua_xmove(State, this->L, 1);
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

int ULuaState::MetaTableFunctionUserData__index(lua_State* L)
{

	ULuaState* LuaState = ULuaState::GetFromExtraSpace(L);
	FLuaUserData* UserData = (FLuaUserData*)lua_touserdata(L, 1);

	if (!UserData->Context.IsValid())
	{
		return luaL_error(L, "invalid UObject for UserData %p", UserData);
	}

	TMap<FString, FLuaValue>* TablePtr = nullptr;
	UObject* Context = UserData->Context.Get();

	ULuaUserDataObject* LuaUserDataObject = nullptr;
	ULuaComponent* LuaComponent = nullptr;

	FString Key = ANSI_TO_TCHAR(lua_tostring(L, 2));

	LuaComponent = Cast<ULuaComponent>(Context);

	if (LuaComponent)
	{
		TablePtr = &LuaComponent->Table;
	}
	else
	{
		LuaUserDataObject = Cast<ULuaUserDataObject>(Context);
		if (LuaUserDataObject)
		{
			TablePtr = &LuaUserDataObject->Table;
		}
	}

	if (TablePtr)
	{
		FLuaValue* LuaValue = TablePtr->Find(Key);
		if (LuaValue)
		{
			LuaState->FromLuaValue(*LuaValue, Context, L);
			return 1;

		}
	}

	if (LuaComponent)
	{
		FLuaValue MetaIndexReturnValue = LuaComponent->ReceiveLuaMetaIndex(Key);
		LuaState->FromLuaValue(MetaIndexReturnValue, Context, L);
		return 1;
	}

	if (LuaUserDataObject)
	{
		FLuaValue MetaIndexReturnValue = LuaUserDataObject->ReceiveLuaMetaIndex(Key);
		LuaState->FromLuaValue(MetaIndexReturnValue, MetaIndexReturnValue.Object ? MetaIndexReturnValue.Object : Context, L);
		return 1;
	}

	lua_pushnil(L);
	return 1;
}

int ULuaState::MetaTableFunctionUserData__newindex(lua_State* L)
{
	ULuaState* LuaState = ULuaState::GetFromExtraSpace(L);
	FLuaUserData* UserData = (FLuaUserData*)lua_touserdata(L, 1);
	if (!UserData->Context.IsValid())
	{
		return luaL_error(L, "invalid UObject for UserData %p", UserData);
	}

	TMap<FString, FLuaValue>* TablePtr = nullptr;
	UObject* Context = UserData->Context.Get();

	ULuaComponent* LuaComponent = Cast<ULuaComponent>(Context);

	if (LuaComponent)
	{
		TablePtr = &LuaComponent->Table;
	}
	else if (ULuaUserDataObject* LuaUserDataObject = Cast<ULuaUserDataObject>(Context))
	{
		TablePtr = &LuaUserDataObject->Table;
	}

	if (TablePtr)
	{
		FString Key = ANSI_TO_TCHAR(lua_tostring(L, 2));

		FLuaValue* LuaValue = TablePtr->Find(Key);
		if (LuaValue)
		{
			*LuaValue = LuaState->ToLuaValue(3, L);
		}
		else
		{
			if (LuaComponent)
			{
				if (LuaComponent->ReceiveLuaMetaNewIndex(LuaState->ToLuaValue(2, L), LuaState->ToLuaValue(3, L)))
				{
					return 0;
				}
			}
			TablePtr->Add(Key, LuaState->ToLuaValue(3, L));
		}
	}

	return 0;
}

FLuaDebug ULuaState::LuaGetInfo(int32 Level)
{
	lua_Debug ar;
	if (lua_getstack(L, Level, &ar) != 1)
		return FLuaDebug();
	lua_getinfo(L, "lSn", &ar);
	FLuaDebug LuaDebug;
	LuaDebug.CurrentLine = ar.currentline;
	LuaDebug.Source = ANSI_TO_TCHAR(ar.source);
	LuaDebug.Name = ANSI_TO_TCHAR(ar.name);
	LuaDebug.NameWhat = ANSI_TO_TCHAR(ar.namewhat);
	LuaDebug.What = ANSI_TO_TCHAR(ar.what);

	return LuaDebug;
}

TMap<FString, FLuaValue> ULuaState::LuaGetLocals(int32 Level)
{
	TMap<FString, FLuaValue> ReturnValue;

	lua_Debug ar;
	if (lua_getstack(L, Level, &ar) != 1)
		return ReturnValue;

	int Index = 1;
	const char* name = lua_getlocal(L, &ar, Index);
	while (name)
	{
		FLuaValue LuaValue = ToLuaValue(-1);
		ReturnValue.Add(ANSI_TO_TCHAR(name), LuaValue);
		Pop();
		name = lua_getlocal(L, &ar, ++Index);
	}
	return ReturnValue;
}

void ULuaState::Debug_Hook(lua_State* L, lua_Debug* ar)
{
	ULuaState* LuaState = ULuaState::GetFromExtraSpace(L);
	FLuaDebug LuaDebug;
	lua_getinfo(L, "lSn", ar);
	LuaDebug.CurrentLine = ar->currentline;
	LuaDebug.Source = ANSI_TO_TCHAR(ar->source);
	LuaDebug.Name = ANSI_TO_TCHAR(ar->name);
	LuaDebug.NameWhat = ANSI_TO_TCHAR(ar->namewhat);
	LuaDebug.What = ANSI_TO_TCHAR(ar->what);

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
	case LUA_HOOKCOUNT:
		LuaState->ReceiveLuaCountHook(LuaDebug);
		break;
	default:
		break;
	}
}

int ULuaState::MetaTableFunctionUserData__eq(lua_State* L)
{
	ULuaState* LuaState = ULuaState::GetFromExtraSpace(L);

	FLuaUserData* UserData = (FLuaUserData*)lua_touserdata(L, 1);
	if (!UserData->Context.IsValid())
	{
		return luaL_error(L, "invalid UObject for UserData %p", UserData);
	}

	FLuaUserData* UserData2 = (FLuaUserData*)lua_touserdata(L, 2);
	if (!UserData2->Context.IsValid())
	{
		return luaL_error(L, "invalid UObject for UserData %p", UserData2);
	}

	if (UserData->Type == UserData2->Type && UserData->Context.Get() == UserData2->Context.Get())
	{
		if (UserData->Type == ELuaValueType::UFunction)
		{
			if (!UserData->Function.IsValid())
			{
				return luaL_error(L, "invalid UFunction for UserData %p", UserData);
			}
			if (!UserData2->Function.IsValid())
			{
				return luaL_error(L, "invalid UFunction for UserData %p", UserData2);
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

int ULuaState::MetaTableFunctionUserData__gc(lua_State* L)
{
	ULuaState* LuaState = ULuaState::GetFromExtraSpace(L);

	FLuaUserData* UserData = (FLuaUserData*)lua_touserdata(L, 1);
	if (!UserData->Context.IsValid())
	{
		return luaL_error(L, "invalid UObject for UserData %p", UserData);
	}

	ULuaUserDataObject* LuaUserDataObject = Cast<ULuaUserDataObject>(UserData->Context.Get());
	if (LuaUserDataObject)
	{
		LuaState->TrackedLuaUserDataObjects.Remove(LuaUserDataObject);
		LuaUserDataObject->ReceiveLuaGC();
	}

	lua_pushnil(L);
	return 1;
}

int ULuaState::MetaTableFunction__call(lua_State* L)
{
	ULuaState* LuaState = ULuaState::GetFromExtraSpace(L);
	FLuaUserData* LuaCallContext = (FLuaUserData*)lua_touserdata(L, 1);

	if (!LuaCallContext->Context.IsValid() || !LuaCallContext->Function.IsValid())
	{
		return luaL_error(L, "invalid lua UFunction for UserData %p", LuaCallContext);
	}

	int NArgs = lua_gettop(L);

	UObject* CallScope = LuaCallContext->Context.Get();
	bool bImplicitSelf = false;
	int StackPointer = 2;

	if (ULuaComponent* LuaComponent = Cast<ULuaComponent>(CallScope))
	{
		CallScope = LuaComponent->GetOwner();
		if (NArgs > 0)
		{
			FLuaValue LuaFirstArgument = LuaState->ToLuaValue(StackPointer, L);
			if (LuaFirstArgument.Type == ELuaValueType::UObject && LuaFirstArgument.Object == LuaComponent)
			{
				bImplicitSelf = LuaComponent->bImplicitSelf;
			}
		}
	}
	else if (ULuaUserDataObject* LuaUserDataObject = Cast<ULuaUserDataObject>(CallScope))
	{
		if (NArgs > 0)
		{
			FLuaValue LuaFirstArgument = LuaState->ToLuaValue(StackPointer, L);
			if (LuaFirstArgument.Type == ELuaValueType::UObject && LuaFirstArgument.Object == LuaUserDataObject)
			{
				bImplicitSelf = LuaUserDataObject->bImplicitSelf;
			}
		}
	}

	FScopeCycleCounterUObject ObjectScope(CallScope);
	FScopeCycleCounterUObject FunctionScope(LuaCallContext->Function.Get());

	void* Parameters = FMemory_Alloca(LuaCallContext->Function->ParmsSize);
	FMemory::Memzero(Parameters, LuaCallContext->Function->ParmsSize);

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	for (TFieldIterator<FProperty> It(LuaCallContext->Function.Get()); (It && It->HasAnyPropertyFlags(CPF_Parm)); ++It)
	{
		FProperty* Prop = *It;
#else
	for (TFieldIterator<UProperty> It(LuaCallContext->Function.Get()); (It && It->HasAnyPropertyFlags(CPF_Parm)); ++It)
	{
		UProperty* Prop = *It;
#endif
		if (!Prop->HasAnyPropertyFlags(CPF_ZeroConstructor))
		{
			Prop->InitializeValue_InContainer(Parameters);
		}
	}

	if (bImplicitSelf)
	{
		NArgs--;
		StackPointer++;
	}

	// arguments
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	for (TFieldIterator<FProperty> FArgs(LuaCallContext->Function.Get()); FArgs && ((FArgs->PropertyFlags & (CPF_Parm | CPF_ReturnParm)) == CPF_Parm); ++FArgs)
	{
		FProperty* Prop = *FArgs;
		FStructProperty* LuaProp = CastField<FStructProperty>(Prop);
#else
	for (TFieldIterator<UProperty> FArgs(LuaCallContext->Function.Get()); FArgs && ((FArgs->PropertyFlags & (CPF_Parm | CPF_ReturnParm)) == CPF_Parm); ++FArgs)
	{
		UProperty* Prop = *FArgs;
		UStructProperty* LuaProp = Cast<UStructProperty>(Prop);
#endif
		if (!LuaProp)
		{
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
			FArrayProperty* ArrayProp = CastField<FArrayProperty>(Prop);
			if (ArrayProp)
			{
				LuaProp = CastField<FStructProperty>(ArrayProp->Inner);
#else
			UArrayProperty* ArrayProp = Cast<UArrayProperty>(Prop);
			if (ArrayProp)
			{
				LuaProp = Cast<UStructProperty>(ArrayProp->Inner);
#endif
				if (!LuaProp)
				{
					break;
				}
				if (LuaProp->Struct != FLuaValue::StaticStruct())
				{
					break;
				}
				// start filling the array with the rest of arguments
				int ArgsToProcess = NArgs - StackPointer + 1;
				if (ArgsToProcess < 1)
				{
					break;
				}
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
		{
			break;
		}

		FLuaValue LuaValue = LuaState->ToLuaValue(StackPointer++, L);
		*LuaProp->ContainerPtrToValuePtr<FLuaValue>(Parameters) = LuaValue;
	}

	LuaState->InceptionLevel++;
	CallScope->ProcessEvent(LuaCallContext->Function.Get(), Parameters);
	check(LuaState->InceptionLevel > 0);
	LuaState->InceptionLevel--;

	if (LuaState->InceptionLevel == 0)
	{
		FString Error;
		while (LuaState->InceptionErrors.Dequeue(Error))
		{
			ULuaComponent* LuaComponent = Cast<ULuaComponent>(LuaCallContext->Context);
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
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	for (TFieldIterator<FProperty> FArgs(LuaCallContext->Function.Get()); FArgs; ++FArgs)
	{
		FProperty* Prop = *FArgs;
#else
	for (TFieldIterator<UProperty> FArgs(LuaCallContext->Function.Get()); FArgs; ++FArgs)
	{
		UProperty* Prop = *FArgs;
#endif
		if (!Prop->HasAnyPropertyFlags(CPF_ReturnParm | CPF_OutParm))
		{
			continue;
		}

		// avoid input args (at all costs !)
		if (Prop->HasAnyPropertyFlags(CPF_ConstParm | CPF_ReferenceParm))
		{
			continue;
		}
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
		FStructProperty* LuaProp = CastField<FStructProperty>(Prop);
		if (!LuaProp)
		{
			FArrayProperty* ArrayProp = CastField<FArrayProperty>(Prop);
			if (ArrayProp)
			{
				LuaProp = CastField<FStructProperty>(ArrayProp->Inner);
#else
		UStructProperty* LuaProp = Cast<UStructProperty>(Prop);
		if (!LuaProp)
		{
			UArrayProperty* ArrayProp = Cast<UArrayProperty>(Prop);
			if (ArrayProp)
			{
				LuaProp = Cast<UStructProperty>(ArrayProp->Inner);
#endif
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

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	for (TFieldIterator<FProperty> It(LuaCallContext->Function.Get()); (It && It->HasAnyPropertyFlags(CPF_Parm)); ++It)
#else
	for (TFieldIterator<UProperty> It(LuaCallContext->Function.Get()); (It && It->HasAnyPropertyFlags(CPF_Parm)); ++It)
#endif
	{
		It->DestroyValue_InContainer(Parameters);
	}


	if (ReturnedValues > 0)
		return ReturnedValues;

	lua_pushnil(L);
	return 1;
}

int ULuaState::MetaTableFunction__rawcall(lua_State * L)
{
	ULuaState* LuaState = ULuaState::GetFromExtraSpace(L);
	FLuaUserData* LuaCallContext = (FLuaUserData*)lua_touserdata(L, 1);

	if (!LuaCallContext->Context.IsValid() || !LuaCallContext->Function.IsValid())
	{
		return luaL_error(L, "invalid lua UFunction for UserData %p", LuaCallContext);
	}

	int NArgs = lua_gettop(L);

	UObject* CallScope = LuaCallContext->Context.Get();
	bool bImplicitSelf = false;
	int StackPointer = 2;

	if (ULuaComponent* LuaComponent = Cast<ULuaComponent>(CallScope))
	{
		CallScope = LuaComponent->GetOwner();
		if (NArgs > 0)
		{
			FLuaValue LuaFirstArgument = LuaState->ToLuaValue(StackPointer, L);
			if (LuaFirstArgument.Type == ELuaValueType::UObject && LuaFirstArgument.Object == LuaComponent)
			{
				bImplicitSelf = LuaComponent->bImplicitSelf;
			}
		}
	}
	else if (ULuaUserDataObject* LuaUserDataObject = Cast<ULuaUserDataObject>(CallScope))
	{
		if (NArgs > 0)
		{
			FLuaValue LuaFirstArgument = LuaState->ToLuaValue(StackPointer, L);
			if (LuaFirstArgument.Type == ELuaValueType::UObject && LuaFirstArgument.Object == LuaUserDataObject)
			{
				bImplicitSelf = LuaUserDataObject->bImplicitSelf;
			}
		}
	}

	FScopeCycleCounterUObject ObjectScope(CallScope);
	FScopeCycleCounterUObject FunctionScope(LuaCallContext->Function.Get());

	void* Parameters = FMemory_Alloca(LuaCallContext->Function->ParmsSize);
	FMemory::Memzero(Parameters, LuaCallContext->Function->ParmsSize);

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	for (TFieldIterator<FProperty> It(LuaCallContext->Function.Get()); (It && It->HasAnyPropertyFlags(CPF_Parm)); ++It)
	{
		FProperty* Prop = *It;
#else
	for (TFieldIterator<UProperty> It(LuaCallContext->Function.Get()); (It && It->HasAnyPropertyFlags(CPF_Parm)); ++It)
	{
		UProperty* Prop = *It;
#endif
		if (!Prop->HasAnyPropertyFlags(CPF_ZeroConstructor))
		{
			Prop->InitializeValue_InContainer(Parameters);
		}
	}

	if (bImplicitSelf)
	{
		NArgs--;
		StackPointer++;
	}

	// arguments
#if ENGINE_MAJOR_VERSION > 4 ||  ENGINE_MINOR_VERSION >= 25
	for (TFieldIterator<FProperty> FArgs(LuaCallContext->Function.Get()); FArgs && ((FArgs->PropertyFlags & (CPF_Parm | CPF_ReturnParm)) == CPF_Parm); ++FArgs)
	{
		FProperty* Prop = *FArgs;
#else
	for (TFieldIterator<UProperty> FArgs(LuaCallContext->Function.Get()); FArgs && ((FArgs->PropertyFlags & (CPF_Parm | CPF_ReturnParm)) == CPF_Parm); ++FArgs)
	{
		UProperty* Prop = *FArgs;
#endif
		bool bPropertySet = false;
		LuaState->ToProperty(Parameters, Prop, LuaState->ToLuaValue(StackPointer++, L), bPropertySet, 0);
	}

	LuaState->InceptionLevel++;
	CallScope->ProcessEvent(LuaCallContext->Function.Get(), Parameters);
	check(LuaState->InceptionLevel > 0);
	LuaState->InceptionLevel--;

	if (LuaState->InceptionLevel == 0)
	{
		FString Error;
		while (LuaState->InceptionErrors.Dequeue(Error))
		{
			ULuaComponent* LuaComponent = Cast<ULuaComponent>(LuaCallContext->Context);
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
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	for (TFieldIterator<FProperty> FArgs(LuaCallContext->Function.Get()); FArgs; ++FArgs)
	{
		FProperty* Prop = *FArgs;
#else
	for (TFieldIterator<UProperty> FArgs(LuaCallContext->Function.Get()); FArgs; ++FArgs)
	{
		UProperty* Prop = *FArgs;
#endif
		if (!Prop->HasAnyPropertyFlags(CPF_ReturnParm | CPF_OutParm))
		{
			continue;
		}

		// avoid input args (at all costs !)
		if (Prop->HasAnyPropertyFlags(CPF_ConstParm | CPF_ReferenceParm))
		{
			continue;
		}

		bool bPropertyGet = false;
		FLuaValue LuaValue = LuaState->FromProperty(Parameters, Prop, bPropertyGet, 0);
		ReturnedValues++;
		LuaState->FromLuaValue(LuaValue, nullptr, L);
	}

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	for (TFieldIterator<FProperty> It(LuaCallContext->Function.Get()); (It && It->HasAnyPropertyFlags(CPF_Parm)); ++It)
#else
	for (TFieldIterator<UProperty> It(LuaCallContext->Function.Get()); (It && It->HasAnyPropertyFlags(CPF_Parm)); ++It)
#endif
	{
		It->DestroyValue_InContainer(Parameters);
	}

	if (ReturnedValues > 0)
	{
		return ReturnedValues;
	}

	lua_pushnil(L);
	return 1;
}

int ULuaState::MetaTableFunction__rawbroadcast(lua_State * L)
{
	ULuaState* LuaState = ULuaState::GetFromExtraSpace(L);
	FLuaUserData* LuaCallContext = (FLuaUserData*)lua_touserdata(L, 1);

	if (!LuaCallContext->MulticastScriptDelegate || !LuaCallContext->Function.IsValid())
	{
		return luaL_error(L, "invalid lua Multicast Delegate for UserData %p", LuaCallContext);
	}

	int NArgs = lua_gettop(L);
	int StackPointer = 2;

	FScopeCycleCounterUObject FunctionScope(LuaCallContext->Function.Get());

	void* Parameters = FMemory_Alloca(LuaCallContext->Function->ParmsSize);
	FMemory::Memzero(Parameters, LuaCallContext->Function->ParmsSize);

	for (TFieldIterator<FProperty> It(LuaCallContext->Function.Get()); (It && It->HasAnyPropertyFlags(CPF_Parm)); ++It)
	{
		FProperty* Prop = *It;
		if (!Prop->HasAnyPropertyFlags(CPF_ZeroConstructor))
		{
			Prop->InitializeValue_InContainer(Parameters);
		}
	}

	// arguments
	for (TFieldIterator<FProperty> FArgs(LuaCallContext->Function.Get()); FArgs && ((FArgs->PropertyFlags & (CPF_Parm | CPF_ReturnParm)) == CPF_Parm); ++FArgs)
	{
		FProperty* Prop = *FArgs;
		bool bPropertySet = false;
		LuaState->ToProperty(Parameters, Prop, LuaState->ToLuaValue(StackPointer++, L), bPropertySet, 0);
	}

	LuaState->InceptionLevel++;
	LuaCallContext->MulticastScriptDelegate->ProcessMulticastDelegate<UObject>(Parameters);
	check(LuaState->InceptionLevel > 0);
	LuaState->InceptionLevel--;

	if (LuaState->InceptionLevel == 0)
	{
		FString Error;
		while (LuaState->InceptionErrors.Dequeue(Error))
		{
			if (LuaState->bLogError)
			{
				LuaState->LogError(Error);
			}
			LuaState->ReceiveLuaError(Error);
		}
	}

	// no return values in multicast delegates
	for (TFieldIterator<FProperty> It(LuaCallContext->Function.Get()); (It && It->HasAnyPropertyFlags(CPF_Parm)); ++It)
	{
		It->DestroyValue_InContainer(Parameters);
	}

	lua_pushnil(L);
	return 1;
}

int ULuaState::TableFunction_print(lua_State * L)
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
		const char* s = lua_tostring(L, -1);
		if (!s)
			return luaL_error(L, "'tostring must return a string to 'print'");
		FString Value = ANSI_TO_TCHAR(s);
		lua_pop(L, 1);
		Messages.Add(Value);
	}
	LuaState->Log(FString::Join(Messages, TEXT("\t")));
	return 0;
}

int ULuaState::TableFunction_package_loader_codeasset(lua_State * L)
{
	ULuaState* LuaState = ULuaState::GetFromExtraSpace(L);

	// use the second (sanitized by the loader) argument
	FString Key = ANSI_TO_TCHAR(lua_tostring(L, 2));

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION > 0
	FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(Key);
	if (AssetData.IsValid() && AssetData.AssetClassPath.ToString() == "LuaCode")
#else
	FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(*Key);
	if (AssetData.IsValid() && AssetData.AssetClass == "LuaCode")
#endif
	{
		ULuaCode* LuaCode = Cast<ULuaCode>(AssetData.GetAsset());
		if (LuaCode)
		{
			if (!LuaState->RunCodeAsset(LuaCode, 1))
			{
				return luaL_error(L, "%s", lua_tostring(L, -1));
			}
			return 1;
		}
	}

	return luaL_error(L, "unable to load asset '%s'", TCHAR_TO_UTF8(*Key));
}

int ULuaState::TableFunction_package_loader_asset(lua_State * L)
{
	ULuaState* LuaState = ULuaState::GetFromExtraSpace(L);

	// use the second (sanitized by the loader) argument
	const FString Key = ANSI_TO_TCHAR(lua_tostring(L, 2));

	if (LuaState->RunFile(Key, true, 1))
	{
		return 1;
	}
	return luaL_error(L, "%s", lua_tostring(L, -1));
}

int ULuaState::TableFunction_package_loader(lua_State * L)
{
	ULuaState* LuaState = ULuaState::GetFromExtraSpace(L);

	FString Key = ANSI_TO_TCHAR(lua_tostring(L, 1));

	// check for code assets
	if (FPackageName::IsValidObjectPath(Key))
	{
		// fix asset name (if required)
		int32 LastSlashIndex = -1;
		Key.FindLastChar('/', LastSlashIndex);
		const FString LastPart = Key.RightChop(LastSlashIndex + 1);
		if (!LastPart.Contains("."))
		{
			Key = FString::Printf(TEXT("%s.%s"), *Key, *LastPart);
		}

		lua_pushcfunction(L, ULuaState::TableFunction_package_loader_codeasset);
		lua_pushstring(L, TCHAR_TO_UTF8(*Key));
		return 2;
	}
	else
	{
		// TODO: make it smarter by checking for file extension...
		if (!Key.EndsWith(".lua"))
		{
			Key += ".lua";
		}
		// search in root content...
		FString AbsoluteFilename = FPaths::Combine(FPaths::ProjectContentDir(), Key);
		if (FPaths::FileExists(AbsoluteFilename))
		{
			lua_pushcfunction(L, ULuaState::TableFunction_package_loader_asset);
			lua_pushstring(L, TCHAR_TO_UTF8(*Key));
			return 2;
		}
		else
		{
			// or search in additional paths
			for (FString AdditionalPath : LuaState->AppendProjectContentDirSubDir)
			{
				AbsoluteFilename = FPaths::Combine(FPaths::ProjectContentDir(), AdditionalPath, Key);
				if (FPaths::FileExists(AbsoluteFilename))
				{
					lua_pushcfunction(L, ULuaState::TableFunction_package_loader_asset);
					lua_pushstring(L, TCHAR_TO_UTF8(*(AdditionalPath / Key)));
					return 2;
				}
			}
		}
	}

	// use UTF8 as the package name can contains non-ASCII chars
	lua_pushstring(L, TCHAR_TO_UTF8(*FString::Printf(TEXT("\n\tno asset '%s'"), *Key)));
	return 1;
}

int ULuaState::TableFunction_package_preload(lua_State * L)
{
	ULuaState* LuaState = ULuaState::GetFromExtraSpace(L);

	if (LuaState->L != L)
	{
		return luaL_error(L, "you cannot call package.preload from a thread/coroutine (error while loading %s)", lua_tostring(L, 1));
	}

	FString Key = ANSI_TO_TCHAR(lua_tostring(L, 1));

	// check for code assets
	ULuaCode** LuaCodePtr = LuaState->RequireTable.Find(Key);
	if (!LuaCodePtr)
	{
		if (LuaState->bAddProjectContentDirToPackagePath && LuaState->RunFile(Key + ".lua", true, 1))
		{
			return 1;
		}

		// now search in additional paths
		for (FString AdditionalPath : LuaState->AppendProjectContentDirSubDir)
		{
			if (LuaState->RunFile(AdditionalPath / Key + ".lua", true, 1))
			{
				return 1;
			}
			return luaL_error(L, "%s", lua_tostring(L, -1));

		}
		return luaL_error(L, "unable to find package %s", TCHAR_TO_ANSI(*Key));
	}

	ULuaCode* LuaCode = *LuaCodePtr;
	if (!LuaCode)
	{
		return luaL_error(L, "LuaCodeAsset not set for package %s", TCHAR_TO_ANSI(*Key));
	}

	if (!LuaState->RunCodeAsset(LuaCode, 1))
	{
		return luaL_error(L, "%s", lua_tostring(L, -1));
	}

	return 1;
}

void ULuaState::ReceiveLuaError_Implementation(const FString & Message)
{

}

void ULuaState::ReceiveLuaCallHook_Implementation(const FLuaDebug & LuaDebug)
{

}

void ULuaState::ReceiveLuaReturnHook_Implementation(const FLuaDebug & LuaDebug)
{

}

void ULuaState::ReceiveLuaLineHook_Implementation(const FLuaDebug & LuaDebug)
{

}

void ULuaState::ReceiveLuaCountHook(const FLuaDebug & LuaDebug)
{

}

void ULuaState::ReceiveLuaLevelRemovedFromWorld_Implementation(ULevel * Level, UWorld * World)
{

}

void ULuaState::ReceiveLuaLevelAddedToWorld_Implementation(ULevel * Level, UWorld * World)
{

}

void ULuaState::ReceiveLuaStatePreInitialized_Implementation()
{

}

void ULuaState::ReceiveLuaStateInitialized_Implementation()
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

void ULuaState::PushRegistryTable()
{
	lua_pushvalue(L, LUA_REGISTRYINDEX);
}

int32 ULuaState::GetFieldFromTree(const FString & Tree, bool bGlobal)
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
		GetField(-1, TCHAR_TO_ANSI(*Parts[i]));

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

void ULuaState::SetFieldFromTree(const FString & Tree, FLuaValue & Value, bool bGlobal, UObject * CallContext)
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
	FromLuaValue(Value, CallContext);
	SetField(-2, TCHAR_TO_ANSI(*Parts.Last()));
	Pop(ItemsToPop - 1);
}


void ULuaState::NewUObject(UObject * Object, lua_State * State)
{
	if (!State)
	{
		State = this->L;
	}
	FLuaUserData* UserData = (FLuaUserData*)lua_newuserdata(State, sizeof(FLuaUserData));
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

bool ULuaState::PCall(int NArgs, FLuaValue & Value, int NRet)
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

bool ULuaState::Call(int NArgs, FLuaValue & Value, int NRet)
{
	if (lua_pcall(L, NArgs, NRet, 0))
	{
		LastError = FString::Printf(TEXT("Lua error: %s"), ANSI_TO_TCHAR(lua_tostring(L, -1)));
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

void ULuaState::UnrefChecked(int Ref)
{
	// in case of moved value (like when compiling a blueprint), L should be nullptr
	if (!L)
		return;

	Unref(Ref);
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

bool ULuaState::Yield(int Index, int NArgs)
{
	lua_State* Coroutine = lua_tothread(L, Index);
	if (!Coroutine)
		return false;

	if (lua_status(Coroutine) == LUA_OK && lua_gettop(Coroutine) == 0)
	{
		lua_pushboolean(L, 0);
		lua_pushstring(L, "Lua error: cannot yield a dead coroutine");
		return false;
	}

	lua_xmove(L, Coroutine, NArgs);

	int	Ret = lua_yield(Coroutine, NArgs);

	if (Ret != LUA_OK)
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

int32 ULuaState::ILen(int Index)
{
	return luaL_len(L, Index);
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
	FromLuaValue(NewTable);
	Pop();
	return NewTable;
}

FLuaValue ULuaState::CreateLuaLazyTable()
{
	FLuaValue NewTable;
	NewTable.Type = ELuaValueType::Table;
	NewTable.LuaState = this;
	return NewTable;
}

FLuaValue ULuaState::CreateLuaThread(FLuaValue Value)
{
	FLuaValue NewThread;
	NewThread.Type = ELuaValueType::Thread;
	NewThread.LuaState = this;
	FromLuaValue(NewThread);
	lua_State* NewLuaThread = lua_tothread(L, -1);
	FromLuaValue(Value, nullptr, NewLuaThread);
	Pop();
	return NewThread;
}

ELuaThreadStatus ULuaState::GetLuaThreadStatus(FLuaValue Value)
{
	if (Value.Type != ELuaValueType::Thread || Value.LuaState != this)
		return ELuaThreadStatus::Invalid;

	FromLuaValue(Value);
	lua_State* LuaThread = lua_tothread(L, -1);
	int ReturnValue = lua_status(LuaThread);
	Pop();

	if (ReturnValue == 0)
		return ELuaThreadStatus::Ok;

	if (ReturnValue == LUA_YIELD)
		return ELuaThreadStatus::Suspended;

	return ELuaThreadStatus::Error;
}

int32 ULuaState::GetLuaThreadStackTop(FLuaValue Value)
{
	if (Value.Type != ELuaValueType::Thread || Value.LuaState != this)
		return MIN_int32;

	FromLuaValue(Value);
	lua_State* LuaThread = lua_tothread(L, -1);
	int ReturnValue = lua_gettop(LuaThread);
	Pop();

	return ReturnValue;
}

TSharedRef<FLuaSmartReference> ULuaState::AddLuaSmartReference(FLuaValue Value)
{
	TSharedRef<FLuaSmartReference> Ref = MakeShared<FLuaSmartReference>();
	Ref->LuaState = this;
	Ref->Value = Value;

	LuaSmartReferences.Add(Ref);

	return Ref;
}

void ULuaState::RemoveLuaSmartReference(TSharedRef<FLuaSmartReference> Ref)
{
	LuaSmartReferences.Remove(Ref);
}

ULuaState::~ULuaState()
{
	FCoreUObjectDelegates::GetPostGarbageCollect().Remove(GCLuaDelegatesHandle);

#if WITH_EDITOR
	if (LuaConsole.LuaState)
	{
		IModularFeatures::Get().UnregisterModularFeature(IConsoleCommandExecutor::ModularFeatureName(), &LuaConsole);
	}
#endif

	FLuaMachineModule::Get().UnregisterLuaState(this);

	if (L)
	{
		lua_close(L);
		L = nullptr;
	}
}

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
#define LUAVALUE_PROP_CAST(Type, Type2) F##Type* __##Type##__ = CastField<F##Type>(Property);\
	if (__##Type##__)\
	{\
		return FLuaValue((Type2)__##Type##__->GetPropertyValue_InContainer(Buffer, Index));\
	}

#define LUAVALUE_PROP_CAST_TOSTRING(Type) F##Type* __##Type##__ = CastField<F##Type>(Property);\
	if (__##Type##__)\
	{\
		return FLuaValue(__##Type##__->GetPropertyValue_InContainer(Buffer, Index).ToString());\
	}

#define LUAVALUE_PROP_SET(Type, Value) F##Type* __##Type##__ = CastField<F##Type>(Property);\
	if (__##Type##__)\
	{\
		__##Type##__->SetPropertyValue_InContainer(Buffer, Value, Index);\
		return;\
	}
#else
#define LUAVALUE_PROP_CAST(Type, Type2) U##Type* __##Type##__ = Cast<U##Type>(Property);\
	if (__##Type##__)\
	{\
		return FLuaValue((Type2)__##Type##__->GetPropertyValue_InContainer(Buffer, Index));\
	}

#define LUAVALUE_PROP_CAST_TOSTRING(Type) U##Type* __##Type##__ = Cast<U##Type>(Property);\
	if (__##Type##__)\
	{\
		return FLuaValue(__##Type##__->GetPropertyValue_InContainer(Buffer, Index).ToString());\
	}

#define LUAVALUE_PROP_SET(Type, Value) U##Type* __##Type##__ = Cast<U##Type>(Property);\
	if (__##Type##__)\
	{\
		__##Type##__->SetPropertyValue_InContainer(Buffer, Value, Index);\
		return;\
	}
#endif

#if	ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
FLuaValue ULuaState::FromUProperty(void* Buffer, FProperty * Property, bool& bSuccess, int32 Index)
{
	return FromFProperty(Buffer, Property, bSuccess, Index);
}
void ULuaState::ToUProperty(void* Buffer, FProperty * Property, FLuaValue Value, bool& bSuccess, int32 Index)
{
	ToFProperty(Buffer, Property, Value, bSuccess, Index);
}
#endif

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
FLuaValue ULuaState::FromFProperty(void* Buffer, FProperty * Property, bool& bSuccess, int32 Index)
#else
FLuaValue ULuaState::FromUProperty(void* Buffer, UProperty * Property, bool& bSuccess, int32 Index)
#endif
{
	bSuccess = true;

	LUAVALUE_PROP_CAST(BoolProperty, bool);
	LUAVALUE_PROP_CAST(DoubleProperty, double);
	LUAVALUE_PROP_CAST(FloatProperty, float);
	LUAVALUE_PROP_CAST(Int64Property, int64);
	LUAVALUE_PROP_CAST(UInt64Property, int64);
	LUAVALUE_PROP_CAST(IntProperty, int32);
	LUAVALUE_PROP_CAST(UInt32Property, int32);
	LUAVALUE_PROP_CAST(Int16Property, int32);
	LUAVALUE_PROP_CAST(Int8Property, int32);
	LUAVALUE_PROP_CAST(ByteProperty, int32);
	LUAVALUE_PROP_CAST(UInt16Property, int32);

	LUAVALUE_PROP_CAST(StrProperty, FString);
	LUAVALUE_PROP_CAST_TOSTRING(NameProperty);
	LUAVALUE_PROP_CAST_TOSTRING(TextProperty);

	LUAVALUE_PROP_CAST(ClassProperty, UObject*);
	LUAVALUE_PROP_CAST(ObjectProperty, UObject*);

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property);

	if (EnumProperty)
	{
		const uint8* EnumValue = EnumProperty->ContainerPtrToValuePtr<const uint8>(Buffer, Index);
		return FLuaValue((int32)*EnumValue);
	}
#endif

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	FObjectPropertyBase* ObjectPropertyBase = CastField<FObjectPropertyBase>(Property);
#else
	UObjectPropertyBase* ObjectPropertyBase = Cast<UObjectPropertyBase>(Property);
#endif
	if (ObjectPropertyBase)
	{
		return FLuaValue(ObjectPropertyBase->GetObjectPropertyValue_InContainer(Buffer, Index));
	}

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	FWeakObjectProperty* WeakObjectProperty = CastField<FWeakObjectProperty>(Property);
#else
	UWeakObjectProperty* WeakObjectProperty = Cast<UWeakObjectProperty>(Property);
#endif
	if (WeakObjectProperty)
	{
		const FWeakObjectPtr& WeakPtr = WeakObjectProperty->GetPropertyValue_InContainer(Buffer, Index);
		return FLuaValue(WeakPtr.Get());
	}

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	if (FMulticastDelegateProperty* MulticastProperty = CastField<FMulticastDelegateProperty>(Property))
#else
	if (UMulticastDelegateProperty* MulticastProperty = Cast<UMulticastDelegateProperty>(Property))
#endif
	{
		FLuaValue MulticastValue;
		MulticastValue.Type = ELuaValueType::MulticastDelegate;
		MulticastValue.Object = MulticastProperty->SignatureFunction;
		MulticastValue.MulticastScriptDelegate = reinterpret_cast<FMulticastScriptDelegate*>(MulticastProperty->ContainerPtrToValuePtr<uint8>(Buffer));
		return MulticastValue;
	}

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	if (FDelegateProperty* DelegateProperty = CastField<FDelegateProperty>(Property))
#else
	if (UDelegateProperty* DelegateProperty = Cast<UDelegateProperty>(Property))
#endif
	{
		const FScriptDelegate& ScriptDelegate = DelegateProperty->GetPropertyValue_InContainer(Buffer, Index);
		return FLuaValue::FunctionOfObject((UObject*)ScriptDelegate.GetUObject(), ScriptDelegate.GetFunctionName());
	}

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	if (FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
#else
	if (UArrayProperty* ArrayProperty = Cast<UArrayProperty>(Property))
#endif
	{
		FLuaValue NewLuaArray = CreateLuaTable();
		FScriptArrayHelper_InContainer Helper(ArrayProperty, Buffer, Index);
		for (int32 ArrayIndex = 0; ArrayIndex < Helper.Num(); ArrayIndex++)
		{
			uint8* ArrayItemPtr = Helper.GetRawPtr(ArrayIndex);
			bool bArrayItemSuccess = false;
			NewLuaArray.SetFieldByIndex(ArrayIndex + 1, FromProperty(ArrayItemPtr, ArrayProperty->Inner, bArrayItemSuccess, 0));
		}
		return NewLuaArray;
	}

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	if (FMapProperty* MapProperty = CastField<FMapProperty>(Property))
#else
	if (UMapProperty* MapProperty = Cast<UMapProperty>(Property))
#endif
	{
		FLuaValue NewLuaTable = CreateLuaTable();
		FScriptMapHelper_InContainer Helper(MapProperty, Buffer, Index);
		for (int32 MapIndex = 0; MapIndex < Helper.Num(); MapIndex++)
		{
			uint8* ArrayKeyPtr = Helper.GetKeyPtr(MapIndex);
			uint8* ArrayValuePtr = Helper.GetValuePtr(MapIndex);
			bool bArrayItemSuccess = false;
			NewLuaTable.SetField(
				FromProperty(ArrayKeyPtr, MapProperty->KeyProp, bArrayItemSuccess, 0).ToString(),
				FromProperty(ArrayValuePtr, MapProperty->ValueProp, bArrayItemSuccess, 0));
		}
		return NewLuaTable;
	}

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	if (FSetProperty* SetProperty = CastField<FSetProperty>(Property))
#else
	if (USetProperty* SetProperty = Cast<USetProperty>(Property))
#endif
	{
		FLuaValue NewLuaArray = CreateLuaTable();
		FScriptSetHelper_InContainer Helper(SetProperty, Buffer, Index);
		for (int32 SetIndex = 0; SetIndex < Helper.Num(); SetIndex++)
		{
			uint8* ArrayItemPtr = Helper.GetElementPtr(SetIndex);
			bool bArrayItemSuccess = false;
			NewLuaArray.SetFieldByIndex(SetIndex + 1, FromProperty(ArrayItemPtr, SetProperty->ElementProp, bArrayItemSuccess, 0));
		}
		return NewLuaArray;
	}

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
#else
	if (UStructProperty* StructProperty = Cast<UStructProperty>(Property))
#endif
	{
		// fast path
		if (StructProperty->Struct == FLuaValue::StaticStruct())
		{
			FLuaValue* LuaValuePtr = StructProperty->ContainerPtrToValuePtr<FLuaValue>(Buffer);
			// trick for allowing lazy tables creation
			FromLuaValue(*LuaValuePtr);
			Pop();
			return *LuaValuePtr;
		}

		const uint8* StructContainer = StructProperty->ContainerPtrToValuePtr<const uint8>(Buffer, Index);

		return StructToLuaTable(StructProperty->Struct, StructContainer);
	}

	bSuccess = false;
	return FLuaValue();
}

FLuaValue ULuaState::StructToLuaTable(UScriptStruct * InScriptStruct, const uint8 * StructData)
{
	FLuaValue NewLuaTable = CreateLuaTable();
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	for (TFieldIterator<FProperty> It(InScriptStruct); It; ++It)
#else
	for (TFieldIterator<UProperty> It(InScriptStruct); It; ++It)
#endif
	{
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
		FProperty* FieldProp = *It;
#else
		UProperty* FieldProp = *It;
#endif
		FString PropName = FieldProp->GetName();
		bool bTableItemSuccess = false;
		NewLuaTable.SetField(PropName, FromProperty((void*)StructData, FieldProp, bTableItemSuccess, 0));
	}
	return NewLuaTable;
}

FLuaValue ULuaState::StructToLuaTable(UScriptStruct * InScriptStruct, const TArray<uint8>&StructData)
{
	return StructToLuaTable(InScriptStruct, StructData.GetData());
}

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
void ULuaState::ToFProperty(void* Buffer, FProperty * Property, FLuaValue Value, bool& bSuccess, int32 Index)
#else
void ULuaState::ToUProperty(void* Buffer, UProperty * Property, FLuaValue Value, bool& bSuccess, int32 Index)
#endif
{
	bSuccess = true;

	LUAVALUE_PROP_SET(BoolProperty, Value.ToBool());
	LUAVALUE_PROP_SET(DoubleProperty, Value.ToFloat());
	LUAVALUE_PROP_SET(FloatProperty, Value.ToFloat());
	LUAVALUE_PROP_SET(IntProperty, Value.ToInteger());
	LUAVALUE_PROP_SET(UInt32Property, Value.ToInteger());
	LUAVALUE_PROP_SET(Int64Property, Value.ToInteger());
	LUAVALUE_PROP_SET(UInt64Property, Value.ToInteger());
	LUAVALUE_PROP_SET(Int16Property, Value.ToInteger());
	LUAVALUE_PROP_SET(Int8Property, Value.ToInteger());
	LUAVALUE_PROP_SET(ByteProperty, Value.ToInteger());
	LUAVALUE_PROP_SET(UInt16Property, Value.ToInteger());

	LUAVALUE_PROP_SET(StrProperty, Value.ToString());
	LUAVALUE_PROP_SET(NameProperty, Value.ToName());
	LUAVALUE_PROP_SET(TextProperty, FText::FromString(Value.ToString()));

	LUAVALUE_PROP_SET(ClassProperty, Value.Object);
	LUAVALUE_PROP_SET(ObjectProperty, Value.Object);

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property);

	if (EnumProperty)
	{
		uint8* EnumValue = EnumProperty->ContainerPtrToValuePtr<uint8>(Buffer, Index);
		*EnumValue = Value.ToInteger();
		return;
	}
#endif

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	FObjectPropertyBase* ObjectPropertyBase = CastField<FObjectPropertyBase>(Property);
#else
	UObjectPropertyBase* ObjectPropertyBase = Cast<UObjectPropertyBase>(Property);
#endif
	if (ObjectPropertyBase)
	{
		ObjectPropertyBase->SetObjectPropertyValue_InContainer(Buffer, Value.Object, Index);
	}

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	FWeakObjectProperty* WeakObjectProperty = CastField<FWeakObjectProperty>(Property);
#else
	UWeakObjectProperty* WeakObjectProperty = Cast<UWeakObjectProperty>(Property);
#endif
	if (WeakObjectProperty)
	{
		FWeakObjectPtr WeakPtr(Value.Object);
		WeakObjectProperty->SetPropertyValue_InContainer(Buffer, WeakPtr, Index);
		return;
	}

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	if (FMulticastDelegateProperty* MulticastProperty = CastField<FMulticastDelegateProperty>(Property))
#else
	if (UMulticastDelegateProperty* MulticastProperty = Cast<UMulticastDelegateProperty>(Property))
#endif
	{
		if (Value.IsNil())
		{
			UObject* Object = static_cast<UObject*>(Buffer);
			UnregisterLuaDelegatesOfObject(Object);
			MulticastProperty->ClearDelegate(Object);
			return;
		}

		ULuaDelegate* LuaDelegate = NewObject<ULuaDelegate>();
		LuaDelegate->SetupLuaDelegate(MulticastProperty->SignatureFunction, this, Value);
		RegisterLuaDelegate((UObject*)Buffer, LuaDelegate);

		FScriptDelegate Delegate;
		Delegate.BindUFunction(LuaDelegate, FName("LuaDelegateFunction"));

		MulticastProperty->AddDelegate(Delegate, (UObject*)Buffer);
		return;
	}

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	if (FDelegateProperty* DelegateProperty = CastField<FDelegateProperty>(Property))
#else
	if (UDelegateProperty* DelegateProperty = Cast<UDelegateProperty>(Property))
#endif
	{
		if (Value.IsNil())
		{
			UObject* Object = static_cast<UObject*>(Buffer);
			UnregisterLuaDelegatesOfObject(Object);
			DelegateProperty->SetPropertyValue_InContainer(Buffer, FScriptDelegate(), Index);
			return;
		}

		ULuaDelegate* LuaDelegate = NewObject<ULuaDelegate>();
		LuaDelegate->SetupLuaDelegate(DelegateProperty->SignatureFunction, this, Value);
		RegisterLuaDelegate((UObject*)Buffer, LuaDelegate);

		FScriptDelegate Delegate;
		Delegate.BindUFunction(LuaDelegate, FName("LuaDelegateFunction"));

		DelegateProperty->SetPropertyValue_InContainer(Buffer, Delegate, Index);
		return;
	}

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
#else
	if (UStructProperty* StructProperty = Cast<UStructProperty>(Property))
#endif
	{
		// fast path
		if (StructProperty->Struct == FLuaValue::StaticStruct())
		{
			FLuaValue* LuaValuePtr = StructProperty->ContainerPtrToValuePtr<FLuaValue>(Buffer);
			*LuaValuePtr = Value;
			return;
		}

		const uint8* StructContainer = StructProperty->ContainerPtrToValuePtr<const uint8>(Buffer, Index);
		LuaTableToStruct(Value, StructProperty->Struct, (uint8*)StructContainer);
		return;
	}

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	if (FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
#else
	if (UArrayProperty* ArrayProperty = Cast<UArrayProperty>(Property))
#endif
	{
		FScriptArrayHelper_InContainer Helper(ArrayProperty, Buffer, Index);
		TArray<FLuaValue> ArrayValues = ULuaBlueprintFunctionLibrary::LuaTableGetValues(Value);
		Helper.Resize(ArrayValues.Num());
		for (int32 ArrayIndex = 0; ArrayIndex < Helper.Num(); ArrayIndex++)
		{
			uint8* ArrayItemPtr = Helper.GetRawPtr(ArrayIndex);
			bool bArrayItemSuccess = false;
			ToProperty(ArrayItemPtr, ArrayProperty->Inner, ArrayValues[ArrayIndex], bArrayItemSuccess, 0);
		}
		return;
	}

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	if (FMapProperty* MapProperty = CastField<FMapProperty>(Property))
#else
	if (UMapProperty* MapProperty = Cast<UMapProperty>(Property))
#endif
	{
		FScriptMapHelper_InContainer Helper(MapProperty, Buffer, Index);
		Helper.EmptyValues();
		TArray<FLuaValue> TableKeys = ULuaBlueprintFunctionLibrary::LuaTableGetKeys(Value);
		for (FLuaValue TableKey : TableKeys)
		{
			int32 NewIndex = Helper.AddUninitializedValue();
			uint8* KeyBuffer = Helper.GetKeyPtr(NewIndex);
			uint8* ValueBuffer = Helper.GetValuePtr(NewIndex);
			bool bTableItemSuccess = false;
			ToProperty(KeyBuffer, Helper.GetKeyProperty(), TableKey, bTableItemSuccess, 0);
			ToProperty(ValueBuffer, Helper.GetValueProperty(), TableKey, bTableItemSuccess, 0);
		}
		return;
	}

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	if (FSetProperty* SetProperty = CastField<FSetProperty>(Property))
#else
	if (USetProperty* SetProperty = Cast<USetProperty>(Property))
#endif
	{
		FScriptSetHelper_InContainer Helper(SetProperty, Buffer, Index);
		TArray<FLuaValue> ArrayValues = ULuaBlueprintFunctionLibrary::LuaTableGetValues(Value);
		Helper.EmptyElements(ArrayValues.Num());
		for (int32 ArrayIndex = 0; ArrayIndex < Helper.Num(); ArrayIndex++)
		{
			uint8* SetItemPtr = Helper.GetElementPtr(ArrayIndex);
			bool bArrayItemSuccess = false;
			ToProperty(SetItemPtr, SetProperty->ElementProp, ArrayValues[ArrayIndex], bArrayItemSuccess, 0);
		}
		return;
	}

	bSuccess = false;
}

void ULuaState::LuaTableToStruct(FLuaValue & LuaValue, UScriptStruct * InScriptStruct, uint8 * StructData)
{
	TArray<FLuaValue> TableKeys = ULuaBlueprintFunctionLibrary::LuaTableGetKeys(LuaValue);
	for (FLuaValue TableKey : TableKeys)
	{
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
		FProperty* StructProp = InScriptStruct->FindPropertyByName(TableKey.ToName());
#else
		UProperty* StructProp = InScriptStruct->FindPropertyByName(TableKey.ToName());
#endif
		if (StructProp)
		{
			bool bStructValueSuccess = false;
			ToProperty((void*)StructData, StructProp, LuaValue.GetField(TableKey.ToString()), bStructValueSuccess, 0);
		}
	}
}

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
void ULuaState::ToProperty(void* Buffer, FProperty * Property, FLuaValue Value, bool& bSuccess, int32 Index)
{
	ToFProperty(Buffer, Property, Value, bSuccess, Index);
}

FLuaValue ULuaState::FromProperty(void* Buffer, FProperty * Property, bool& bSuccess, int32 Index)
{
	return FromFProperty(Buffer, Property, bSuccess, Index);
}
#else
void ULuaState::ToProperty(void* Buffer, UProperty * Property, FLuaValue Value, bool& bSuccess, int32 Index)
{
	ToUProperty(Buffer, Property, Value, bSuccess, Index);
}
FLuaValue ULuaState::FromProperty(void* Buffer, UProperty * Property, bool& bSuccess, int32 Index)
{
	return FromUProperty(Buffer, Property, bSuccess, Index);
}
#endif


FLuaValue ULuaState::GetLuaValueFromProperty(UObject * InObject, const FString & PropertyName)
{
	if (!InObject)
	{
		return FLuaValue();
	}

	UClass* Class = InObject->GetClass();
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	FProperty* Property = nullptr;
#else
	UProperty* Property = nullptr;
#endif
	Property = Class->FindPropertyByName(*PropertyName);
	if (Property)
	{
		bool bSuccess = false;
		return FromProperty(InObject, Property, bSuccess);
	}

	return FLuaValue();
}

bool ULuaState::SetPropertyFromLuaValue(UObject * InObject, const FString & PropertyName, FLuaValue Value)
{
	if (!InObject)
	{
		return false;
	}

	UClass* Class = InObject->GetClass();
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	FProperty* Property = nullptr;
#else
	UProperty* Property = nullptr;
#endif
	Property = Class->FindPropertyByName(*PropertyName);
	if (Property)
	{
		bool bSuccess = false;
		ToProperty(InObject, Property, Value, bSuccess);
		return bSuccess;
	}

	return false;
}

void ULuaState::SetUserDataMetaTable(FLuaValue MetaTable)
{
	UserDataMetaTable = MetaTable;
}

void ULuaState::SetupAndAssignUserDataMetatable(UObject * Context, TMap<FString, FLuaValue>&Metatable, lua_State * State)
{
	if (!State)
	{
		State = this->L;
	}

	lua_newtable(State);
	lua_pushcfunction(State, ULuaState::MetaTableFunctionUserData__index);
	lua_setfield(State, -2, "__index");
	lua_pushcfunction(State, ULuaState::MetaTableFunctionUserData__newindex);
	lua_setfield(State, -2, "__newindex");
	lua_pushcfunction(State, ULuaState::MetaTableFunctionUserData__eq);
	lua_setfield(State, -2, "__eq");
	if (Context->IsA<ULuaUserDataObject>())
	{
		lua_pushcfunction(State, ULuaState::MetaTableFunctionUserData__gc);
		lua_setfield(State, -2, "__gc");
	}

	for (TPair<FString, FLuaValue>& Pair : Metatable)
	{
		// first check for UFunction
		if (Pair.Value.Type == ELuaValueType::UFunction)
		{
			UObject* FunctionOwner = Context;
			if (ULuaComponent* LuaComponent = Cast<ULuaComponent>(Context))
			{
				FunctionOwner = LuaComponent->GetOwner();
			}

			if (FunctionOwner)
			{
				UFunction* Function = FunctionOwner->FindFunction(Pair.Value.FunctionName);
				if (Function)
				{
					FLuaUserData* LuaCallContext = (FLuaUserData*)lua_newuserdata(State, sizeof(FLuaUserData));
					LuaCallContext->Type = ELuaValueType::UFunction;
					LuaCallContext->Context = Context;
					LuaCallContext->Function = Function;

					lua_newtable(State);
					lua_pushcfunction(State, bRawLuaFunctionCall ? ULuaState::MetaTableFunction__rawcall : ULuaState::MetaTableFunction__call);
					lua_setfield(State, -2, "__call");
					lua_setmetatable(State, -2);
				}
				else
				{
					lua_pushnil(State);
				}
			}
		}
		else {
			FromLuaValue(Pair.Value, nullptr, State);
		}
		lua_setfield(State, -2, TCHAR_TO_ANSI(*Pair.Key));
	}

	lua_setmetatable(State, -2);
}

FLuaValue ULuaState::NewLuaUserDataObject(TSubclassOf<ULuaUserDataObject> LuaUserDataObjectClass, bool bTrackObject)
{
	ULuaUserDataObject* LuaUserDataObject = NewObject<ULuaUserDataObject>(this, LuaUserDataObjectClass);
	if (LuaUserDataObject)
	{
		if (bTrackObject)
		{
			TrackedLuaUserDataObjects.Add(LuaUserDataObject);
		}
		LuaUserDataObject->ReceiveLuaUserDataTableInit();
		return FLuaValue(LuaUserDataObject);
	}

	return FLuaValue();
}

void ULuaState::SetLuaUserDataField(FLuaValue UserData, const FString & Key, FLuaValue Value)
{
	if (UserData.Type != ELuaValueType::UObject || !UserData.Object)
		return;

	if (ULuaComponent* LuaComponent = Cast<ULuaComponent>(UserData.Object))
	{
		LuaComponent->LuaSetField(Key, Value);
		return;
	}

	if (ULuaUserDataObject* LuaUserDataObject = Cast<ULuaUserDataObject>(UserData.Object))
	{
		LuaUserDataObject->LuaSetField(Key, Value);
		return;
	}
}

FLuaValue ULuaState::GetLuaUserDataField(FLuaValue UserData, const FString & Key)
{
	if (UserData.Type != ELuaValueType::UObject || !UserData.Object)
		return FLuaValue();

	if (ULuaComponent* LuaComponent = Cast<ULuaComponent>(UserData.Object))
	{
		return LuaComponent->LuaGetField(Key);
	}

	if (ULuaUserDataObject* LuaUserDataObject = Cast<ULuaUserDataObject>(UserData.Object))
	{
		return LuaUserDataObject->LuaGetField(Key);
	}

	return FLuaValue();
}

const void* ULuaState::ToPointer(int Index)
{
	return lua_topointer(L, Index);
}

void ULuaState::LuaStateInit()
{
}

void ULuaState::GCLuaDelegatesCheck()
{
	TSet<TWeakObjectPtr<UObject>> DeadObjects;
	for (TPair<TWeakObjectPtr<UObject>, FLuaDelegateGroup>& Pair : LuaDelegatesMap)
	{
		if (!Pair.Key.IsValid())
		{
			DeadObjects.Add(Pair.Key);
		}
	}

	for (TWeakObjectPtr<UObject>& WeakObjectPtr : DeadObjects)
	{
		LuaDelegatesMap.Remove(WeakObjectPtr);
	}
}

void ULuaState::RegisterLuaDelegate(UObject * InObject, ULuaDelegate * InLuaDelegate)
{
	FLuaDelegateGroup* LuaDelegateGroup = LuaDelegatesMap.Find(InObject);
	if (LuaDelegateGroup)
	{
		LuaDelegateGroup->LuaDelegates.Add(InLuaDelegate);
	}
	else
	{
		FLuaDelegateGroup NewLuaDelegateGroup;
		NewLuaDelegateGroup.LuaDelegates.Add(InLuaDelegate);
		LuaDelegatesMap.Add(InObject, NewLuaDelegateGroup);
	}
}

void ULuaState::UnregisterLuaDelegatesOfObject(UObject* InObject)
{
	LuaDelegatesMap.Remove(InObject);
}

TArray<FString> ULuaState::GetPropertiesNames(UObject * InObject)
{
	TArray<FString> Names;

	if (!InObject)
	{
		return Names;
	}

	UClass* Class = InObject->GetClass();
	if (!Class)
	{
		return Names;
	}

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	for (TFieldIterator<FProperty> It(Class); It; ++It)
#else
	for (TFieldIterator<UProperty> It(Class); It; ++It)
#endif
	{
		Names.Add((*It)->GetName());
	}

	return Names;
}

TArray<FString> ULuaState::GetFunctionsNames(UObject * InObject)
{
	TArray<FString> Names;

	if (!InObject)
	{
		return Names;
	}

	UClass* Class = InObject->GetClass();
	if (!Class)
	{
		return Names;
	}

	for (TFieldIterator<UFunction> It(Class); It; ++It)
	{
		Names.Add((*It)->GetName());
	}

	return Names;
}

void ULuaState::AddLuaValueToLuaState(const FString & Name, FLuaValue LuaValue)
{
	SetFieldFromTree(Name, LuaValue, true);
}

FLuaValue ULuaState::RunString(const FString & CodeString, FString CodePath)
{
	FLuaValue ReturnValue;
	if (CodePath.IsEmpty())
	{
		CodePath = CodeString;
	}

	if (!RunCode(CodeString, CodePath, 1))
	{
		if (bLogError)
			LogError(LastError);
		ReceiveLuaError(LastError);
	}
	else
	{
		ReturnValue = ToLuaValue(-1);
	}

	Pop();
	return ReturnValue;
}

void ULuaState::Error(const FString& ErrorString)
{
	luaL_error(L, TCHAR_TO_UTF8(*ErrorString));
}