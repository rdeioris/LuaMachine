// Copyright 2018-2023 - Roberto De Ioris
// Portions Copyright 2025 - Fusion Point Studios, Inc.

#include "LuauState.h"
#include "LuauComponent.h"
#include "LuauUserDataObject.h"
#include "LuauMachine.h"
#include "LuauBlueprintPackage.h"
#include "LuauBlueprintFunctionLibrary.h"
#include "ThirdParty/LuauLibrary/Compiler/include/Luau/Compiler.h"
#include "ThirdParty/LuauLibrary/Compiler/include/Luau/BytecodeBuilder.h"
#include "ThirdParty/LuauLibrary/AST/include/Luau/StringUtils.h"
#include "ThirdParty/LuauLibrary/VM/src/lstate.h"
#include "ThirdParty/LuauLibrary/VM/src/ldebug.h"
#include "ThirdParty/LuauLibrary/VM/src/lobject.h"
#include "ThirdParty/LuauLibrary/VM/src/lfunc.h"
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

LUAUMACHINE_API DEFINE_LOG_CATEGORY(LogLuauMachine);

ULuauState::ULuauState()
{
	L = nullptr;
	bLuauOpenLibs = true;
	bDisabled = false;
	bLogError = true;
	bAddProjectContentDirToPackagePath = true;
	bPersistent = false;
	bEnableLineHook = false;
	bEnableCallHook = false;
	bEnableReturnHook = false;
	bEnableCountHook = false;
	bRawLuauFunctionCall = false;

	FCoreUObjectDelegates::GetPostGarbageCollect().AddUObject(this, &ULuauState::GCLuauDelegatesCheck);
}

ULuauState* ULuauState::GetLuauState(UWorld* InWorld)
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
	lua_setthreaddata(L, this);

	if (bLuauOpenLibs)
	{
		luaL_openlibs(L);
	}

	// get the global table
	lua_getfield(L, LUA_REGISTRYINDEX, "_G");
	// override print
	PushCFunction(ULuauState::TableFunction_print);
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
		const char* CurrentLuauPath = lua_tostring(L, -1);
		FString NewPackagePath = FString(CurrentLuauPath) + ";" + FPaths::ProjectContentDir() + "/?.lua";
		Pop();
		lua_pushstring(L, TCHAR_TO_ANSI(*NewPackagePath));
		SetField(-2, "path");
	}

	for (FString SubDir : AppendProjectContentDirSubDir)
	{
		GetField(-1, "path");
		const char* CurrentLuauPath = lua_tostring(L, -1);
		FString NewPackagePath = FString(CurrentLuauPath) + ";" + FPaths::ProjectContentDir() / SubDir + "/?.lua";
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
	for (TPair<FString, ULuauCode*>& Pair : RequireTable)
	{
		PushCFunction(ULuauState::TableFunction_package_preload);
		SetField(-2, TCHAR_TO_ANSI(*Pair.Key));
	}

	// pop package.prelod
	Pop(1);

	// manage searchers
	GetField(-1, "searchers");
	PushCFunction(ULuauState::TableFunction_package_loader);
	constexpr int PackageLoadersFirstAvailableIndex = 5;
	lua_pushinteger(L, PackageLoadersFirstAvailableIndex);
	lua_insert(L, -3);
	lua_rawset(L, -2);

	// pop package.searchers (and package)
	Pop(2);


	for (TPair<FString, FLuauValue>& Pair : Table)
	{
		FromLuauValue(Pair.Value, this, L);
		SetField(-2, TCHAR_TO_ANSI(*Pair.Key));
	}

	for (TPair<FString, TSubclassOf<ULuauBlueprintPackage>>& Pair : LuauBlueprintPackagesTable)
	{
		if (Pair.Value)
		{
			NewTable();
			ULuauBlueprintPackage* LuauBlueprintPackage = NewObject<ULuauBlueprintPackage>(this, Pair.Value);
			if (LuauBlueprintPackage)
			{
				for (auto LuauPair : LuauBlueprintPackage->Table)
				{
					FromLuauValue(LuauPair.Value, LuauBlueprintPackage);
					SetField(-2, TCHAR_TO_ANSI(*LuauPair.Key));
				}
				// this avoid the package to be GC'd
				LuauBlueprintPackages.Add(Pair.Key, LuauBlueprintPackage);
				LuauBlueprintPackage->SelfTable = ToLuauValue(-1);
				LuauBlueprintPackage->Init();
				LuauBlueprintPackage->ReceiveInit();
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
	ReceiveLuauStatePreInitialized();

	if (LuauCodeAsset)
	{
		if (!RunCodeAsset(LuauCodeAsset))
		{
			if (bLogError)
				LogError(LastError);
			ReceiveLuauError(LastError);
			bDisabled = true;
			return nullptr;
		}
	}

	if (!LuauFilename.IsEmpty())
	{
		if (!RunFile(LuauFilename, true))
		{
			if (bLogError)
				LogError(LastError);
			ReceiveLuauError(LastError);
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
			ReceiveLuauError(LastError);
			bDisabled = true;
			return nullptr;
		}
		UserDataMetaTable = ToLuauValue(-1);
		Pop();
	}

	LuauStateInit();
	ReceiveLuauStateInitialized();

#if WITH_EDITOR
	if (!(GetFlags() & RF_ClassDefaultObject))
	{
		LuauConsole.LuauState = this;
		IModularFeatures::Get().RegisterModularFeature(IConsoleCommandExecutor::ModularFeatureName(), &LuauConsole);
	}
#endif


	return this;
}

FLuauValue ULuauState::GetLuauBlueprintPackageTable(const FString& PackageName)
{
	if (!LuauBlueprintPackages.Contains(PackageName))
	{
		return FLuauValue();
	}

	return LuauBlueprintPackages[PackageName]->SelfTable;
}

bool ULuauState::RunCodeAsset(ULuauCode* CodeAsset, int NRet)
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

bool ULuauState::RunFile(const FString& Filename, bool bIgnoreNonExistent, int NRet, bool bNonContentDirectory)
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
		FLuauValue LuauLastError = FLuauValue(LastError);
		FromLuauValue(LuauLastError);
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
	FLuauValue LuauLastError = FLuauValue(LastError);
	FromLuauValue(LuauLastError);
	return false;
}

bool ULuauState::RunCode(const FString& Code, const FString& CodePath, int NRet)
{
	TArray<uint8> Bytes;
	Bytes.Append((uint8*)TCHAR_TO_UTF8(*Code), FCStringAnsi::Strlen(TCHAR_TO_UTF8(*Code)));
	return RunCode(Bytes, CodePath, NRet);
}

bool ULuauState::RunCode(const TArray<uint8>& Code, const FString& CodePath, int NRet)
{
	FString FullCodePath = FString("@") + CodePath;

	if (luau_load(L, TCHAR_TO_ANSI(*FullCodePath), (const char*)Code.GetData(), Code.Num(), 0))
	{
		LastError = FString::Printf(TEXT("Luau loading error: %s"), ANSI_TO_TCHAR(lua_tostring(L, -1)));
		return false;
	}
	else
	{

		if (lua_pcall(L, 0, NRet, 0))
		{
			LastError = FString::Printf(TEXT("Luau execution error: %s"), ANSI_TO_TCHAR(lua_tostring(L, -1)));
			return false;
		}
	}

	return true;
}

int ULuauState::ToByteCode_Writer(lua_State* L, const void* Ptr, size_t Size, void* UserData)
{
	TArray<uint8>* Output = (TArray<uint8>*)UserData;
	Output->Append((uint8*)Ptr, Size);
	return 0;
}

TArray<uint8> ULuauState::ToByteCode(const FString& Code, const FString& CodePath, FString& ErrorString)
{
	const TCHAR* CodeRaw = *Code;
	FString FullCodePath = FString("@") + CodePath;
	TArray<uint8> Output;

	Luau::BytecodeBuilder bcb;
	bcb.setDumpFlags(Luau::BytecodeBuilder::Dump_Code);
	Luau::CompileOptions options;
	// 0 - no optimization
	// 1 - baseline optimization level that doesn't prevent debuggability
	// 2 - includes optimizations that harm debuggability such as inlining
	options.optimizationLevel = 1;
	// type information is used to guide native code generation decisions
	// information includes testable types for function arguments, locals, upvalues and some temporaries
	// 0 - generate for native modules
	// 1 - generate for all modules
	options.typeInfoLevel = 0;

	Luau::compileOrThrow(bcb, TCHAR_TO_UTF8(CodeRaw), options);

	const std::string dumpstr = bcb.dumpFunction(0);

	lua_State* L = luaL_newstate();
	ULuauState::ToByteCode_Writer(L, dumpstr.c_str(), dumpstr.size(), &Output);
	lua_close(L);
	return Output;
}

void ULuauState::FromLuauValue(FLuauValue& LuauValue, UObject* CallContext, lua_State* State)
{
	if (!State)
	{
		State = this->L;
	}

	switch (LuauValue.Type)
	{
	case ELuauValueType::Bool:
		lua_pushboolean(State, LuauValue.Bool ? 1 : 0);
		break;
	case ELuauValueType::Integer:
		lua_pushinteger(State, LuauValue.Integer);
		break;
	case ELuauValueType::Number:
		lua_pushnumber(State, LuauValue.Number);
		break;
	case ELuauValueType::String:
	{
		TArray<uint8> Bytes = LuauValue.ToBytes();
		lua_pushlstring(State, (const char*)Bytes.GetData(), Bytes.Num());
	}
	break;
	case ELuauValueType::Table:
		if (LuauValue.LuauRef == LUA_NOREF)
		{
			lua_newtable(State);
			lua_pushvalue(State, -1);
			// hold references in the main state
			LuauValue.LuauRef = lua_ref(this->L, LUA_REGISTRYINDEX);
			LuauValue.LuauState = this;
			break;
		}
		if (this != LuauValue.LuauState)
		{
			lua_pushnil(State);
			break;
		}
		lua_rawgeti(this->L, LUA_REGISTRYINDEX, LuauValue.LuauRef);
		if (this->L != State)
			lua_xmove(this->L, State, 1);
		break;
	case ELuauValueType::Thread:
		if (LuauValue.LuauRef == LUA_NOREF)
		{
			lua_newthread(State);
			lua_pushvalue(State, -1);
			LuauValue.LuauRef = lua_ref(this->L, LUA_REGISTRYINDEX);
			LuauValue.LuauState = this;
			break;
		}
		if (this != LuauValue.LuauState)
		{
			lua_pushnil(State);
			break;
		}
		lua_rawgeti(this->L, LUA_REGISTRYINDEX, LuauValue.LuauRef);
		if (this->L != State)
			lua_xmove(this->L, State, 1);
		break;
	case ELuauValueType::Function:
		if (this != LuauValue.LuauState || LuauValue.LuauRef == LUA_NOREF)
		{
			lua_pushnil(State);
			break;
		}
		lua_rawgeti(this->L, LUA_REGISTRYINDEX, LuauValue.LuauRef);
		if (this->L != State)
			lua_xmove(this->L, State, 1);
		break;
	case ELuauValueType::UObject:
	{
		if (!LuauValue.Object)
		{
			lua_pushnil(State);
			break;
		}

		NewUObject(LuauValue.Object, State);
		if (ULuauComponent* LuauComponent = Cast<ULuauComponent>(LuauValue.Object))
		{
			if (!LuauComponent->LuauState)
			{
				UE_LOG(LogLuauMachine, Warning, TEXT("%s has no associated LuauState"), *LuauComponent->GetFullName());
			}
			// ensure we are in the same LuauState
			else if (LuauComponent->LuauState == GetClass())
			{
				SetupAndAssignUserDataMetatable(LuauComponent, LuauComponent->Metatable, State);
			}
		}
		else if (ULuauUserDataObject* LuauUserDataObject = Cast<ULuauUserDataObject>(LuauValue.Object))
		{
			if (!LuauUserDataObject->GetLuauState())
			{
				UE_LOG(LogLuauMachine, Warning, TEXT("%s has no associated LuauState"), *LuauUserDataObject->GetFullName());
			}
			// ensure we are in the same LuauState
			else if (LuauUserDataObject->GetLuauState() == GetClass())
			{
				SetupAndAssignUserDataMetatable(LuauUserDataObject, LuauUserDataObject->Metatable, State);
			}
		}
		else
		{
			if (UserDataMetaTable.Type == ELuauValueType::Table)
			{
				FromLuauValue(UserDataMetaTable, nullptr, State);
			}
			else
			{
				lua_newtable(State);
				// allow comparison between userdata/UObject/UFunction
				lua_pushcfunction(State, ULuauState::MetaTableFunctionUserData__eq, "MetaTableFunctionUserData__eq");
				lua_setfield(State, -2, "__eq");
			}
			lua_setmetatable(State, -2);
		}
	}
	break;
	case ELuauValueType::UFunction:
		// if no context is assigned to the function, own it !
		if (!LuauValue.LuauState.IsValid())
		{
			LuauValue.LuauState = this;
		}
		if (this != LuauValue.LuauState)
		{
			lua_pushnil(State);
			break;
		}
		// first time we should have a CallContext, then we cache it in the Object field
		if (!CallContext)
		{
			CallContext = LuauValue.Object;
		}
		if (CallContext)
		{
			UObject* FunctionOwner = CallContext;
			if (ULuauComponent* LuauComponent = Cast<ULuauComponent>(CallContext))
			{
				FunctionOwner = LuauComponent->GetOwner();
			}

			if (FunctionOwner)
			{
				UFunction* Function = FunctionOwner->FindFunction(LuauValue.FunctionName);
				if (Function)
				{
					// cache it for context-less calls
					LuauValue.Object = CallContext;
					FLuauUserData* LuauCallContext = (FLuauUserData*)lua_newuserdata(State, sizeof(FLuauUserData));
					LuauCallContext->Type = ELuauValueType::UFunction;
					LuauCallContext->Context = CallContext;
					LuauCallContext->Function = Function;
					lua_newtable(State);
					lua_pushcfunction(State, bRawLuauFunctionCall ? ULuauState::MetaTableFunction__rawcall : ULuauState::MetaTableFunction__call, bRawLuauFunctionCall ? "MetaTableFunction__rawcall" : "MetaTableFunction__call");
					lua_setfield(State, -2, "__call");
					lua_setmetatable(State, -2);
					return;
				}
			}
		}
		// no function found
		lua_pushnil(State);
		break;
	case ELuauValueType::MulticastDelegate:
		// if no context is assigned to the function, own it !
		if (!LuauValue.LuauState.IsValid())
		{
			LuauValue.LuauState = this;
		}

		if (this != LuauValue.LuauState)
		{
			lua_pushnil(State);
			break;
		}
		{
			FLuauUserData* LuauCallContext = (FLuauUserData*)lua_newuserdata(State, sizeof(FLuauUserData));
			LuauCallContext->Type = ELuauValueType::MulticastDelegate;
			LuauCallContext->Function = reinterpret_cast<UFunction*>(LuauValue.Object);
			LuauCallContext->MulticastScriptDelegate = LuauValue.MulticastScriptDelegate;
			lua_newtable(State);
			lua_pushcfunction(State, bRawLuauFunctionCall ? ULuauState::MetaTableFunction__rawbroadcast : ULuauState::MetaTableFunction__rawbroadcast, bRawLuauFunctionCall ? "MetaTableFunction__rawbroadcast" : "MetaTableFunction__rawbroadcast");
			lua_setfield(State, -2, "__call");
			lua_setmetatable(State, -2);
			return;
		}
		break;
	default:
		lua_pushnil(State);
	}
}

namespace
{
	int lua_isinteger_luau(lua_State* L, int index) {
		if (!lua_isnumber(L, index)) return 0;  // Ensure it's a number

		lua_Number num = lua_tonumber(L, index);
		return floor(num) == num;  // True if it has no fractional part
	}
} // namespace `anonymous'

FLuauValue ULuauState::ToLuauValue(int Index, lua_State* State)
{
	if (!State)
	{
		State = this->L;
	}

	FLuauValue LuauValue;

	if (lua_isboolean(State, Index))
	{
		LuauValue = FLuauValue(lua_toboolean(State, Index) != 0);
	}
	else if (lua_type(State, Index) == LUA_TSTRING)
	{
		size_t StringLength = 0;
		const char* String = lua_tolstring(State, Index, &StringLength);
		LuauValue = FLuauValue(String, StringLength);
	}
	else if (lua_isinteger_luau(State, Index))
	{
		LuauValue = FLuauValue(lua_tointeger(State, Index));
	}
	else if (lua_type(State, Index) == LUA_TNUMBER)
	{
		LuauValue = FLuauValue(lua_tonumber(State, Index));
	}
	else if (lua_istable(State, Index))
	{
		lua_pushvalue(State, Index);
		if (State != this->L)
			lua_xmove(State, this->L, 1);
		LuauValue.Type = ELuauValueType::Table;
		LuauValue.LuauState = this;
		LuauValue.LuauRef = lua_ref(this->L, LUA_REGISTRYINDEX);
	}
	else if (lua_isthread(State, Index))
	{
		lua_pushvalue(State, Index);
		if (State != this->L)
			lua_xmove(State, this->L, 1);
		LuauValue.Type = ELuauValueType::Thread;
		LuauValue.LuauState = this;
		LuauValue.LuauRef = lua_ref(this->L, LUA_REGISTRYINDEX);
	}
	else if (lua_isfunction(State, Index))
	{
		lua_pushvalue(State, Index);
		if (State != this->L)
			lua_xmove(State, this->L, 1);
		LuauValue.Type = ELuauValueType::Function;
		LuauValue.LuauState = this;
		LuauValue.LuauRef = lua_ref(this->L, LUA_REGISTRYINDEX);
	}
	else if (lua_isuserdata(State, Index))
	{
		FLuauUserData* UserData = (FLuauUserData*)lua_touserdata(State, Index);
		switch (UserData->Type)
		{
		case(ELuauValueType::UObject):
			if (UserData->Context.IsValid())
			{
				LuauValue.Type = UserData->Type;
				LuauValue.Object = UserData->Context.Get();
				LuauValue.LuauState = this;
			}
			break;
		case(ELuauValueType::UFunction):
			if (UserData->Context.IsValid() && UserData->Function.IsValid())
			{
				LuauValue.Type = UserData->Type;
				LuauValue.FunctionName = UserData->Function->GetFName();
				LuauValue.Object = UserData->Context.Get();
				LuauValue.LuauState = this;
			}
			break;
		}
	}

	return LuauValue;
}

int32 ULuauState::GetTop()
{
	return lua_gettop(L);
}

int ULuauState::MetaTableFunctionUserData__index(lua_State* L)
{

	ULuauState* LuauState = ULuauState::GetFromExtraSpace(L);
	FLuauUserData* UserData = (FLuauUserData*)lua_touserdata(L, 1);

	if (!UserData->Context.IsValid())
	{
		lua_pushnil(L);
		luaL_error(L, "invalid UObject for UserData %p", UserData);
	}

	TMap<FString, FLuauValue>* TablePtr = nullptr;
	UObject* Context = UserData->Context.Get();

	ULuauUserDataObject* LuauUserDataObject = nullptr;
	ULuauComponent* LuauComponent = nullptr;

	FString Key = ANSI_TO_TCHAR(lua_tostring(L, 2));

	LuauComponent = Cast<ULuauComponent>(Context);

	if (LuauComponent)
	{
		TablePtr = &LuauComponent->Table;
	}
	else
	{
		LuauUserDataObject = Cast<ULuauUserDataObject>(Context);
		if (LuauUserDataObject)
		{
			TablePtr = &LuauUserDataObject->Table;
		}
	}

	if (TablePtr)
	{
		FLuauValue* LuauValue = TablePtr->Find(Key);
		if (LuauValue)
		{
			LuauState->FromLuauValue(*LuauValue, Context, L);
			return 1;

		}
	}

	if (LuauComponent)
	{
		FLuauValue MetaIndexReturnValue = LuauComponent->ReceiveLuauMetaIndex(Key);
		LuauState->FromLuauValue(MetaIndexReturnValue, Context, L);
		return 1;
	}

	if (LuauUserDataObject)
	{
		FLuauValue MetaIndexReturnValue = LuauUserDataObject->ReceiveLuauMetaIndex(Key);
		LuauState->FromLuauValue(MetaIndexReturnValue, MetaIndexReturnValue.Object ? MetaIndexReturnValue.Object : Context, L);
		return 1;
	}

	lua_pushnil(L);
	return 1;
}

int ULuauState::MetaTableFunctionUserData__newindex(lua_State* L)
{
	ULuauState* LuauState = ULuauState::GetFromExtraSpace(L);
	FLuauUserData* UserData = (FLuauUserData*)lua_touserdata(L, 1);
	if (!UserData->Context.IsValid())
	{
		luaL_error(L, "invalid UObject for UserData %p", UserData);
	}

	TMap<FString, FLuauValue>* TablePtr = nullptr;
	UObject* Context = UserData->Context.Get();

	ULuauComponent* LuauComponent = Cast<ULuauComponent>(Context);

	if (LuauComponent)
	{
		TablePtr = &LuauComponent->Table;
	}
	else if (ULuauUserDataObject* LuauUserDataObject = Cast<ULuauUserDataObject>(Context))
	{
		TablePtr = &LuauUserDataObject->Table;
	}

	if (TablePtr)
	{
		FString Key = ANSI_TO_TCHAR(lua_tostring(L, 2));

		FLuauValue* LuauValue = TablePtr->Find(Key);
		if (LuauValue)
		{
			*LuauValue = LuauState->ToLuauValue(3, L);
		}
		else
		{
			if (LuauComponent)
			{
				if (LuauComponent->ReceiveLuauMetaNewIndex(LuauState->ToLuauValue(2, L), LuauState->ToLuauValue(3, L)))
				{
					return 0;
				}
			}
			TablePtr->Add(Key, LuauState->ToLuauValue(3, L));
		}
	}

	return 0;
}

namespace
{
	int lua_getstack(lua_State* L, int level, lua_Debug* ar) {
		return lua_getinfo(L, level, ">", ar);
	}
} // namespace `anonymous'

FLuauDebug ULuauState::LuauGetInfo(int32 Level)
{
	lua_Debug ar;
	if (lua_getstack(L, Level, &ar) != 1)
		return FLuauDebug();
	lua_getinfo(L, Level, "lSn", &ar);
	FLuauDebug LuauDebug;
	LuauDebug.CurrentLine = ar.currentline;
	LuauDebug.Source = ANSI_TO_TCHAR(ar.source);
	LuauDebug.Name = ANSI_TO_TCHAR(ar.name);
	//LuauDebug.NameWhat = ANSI_TO_TCHAR(ar->namewhat);
	LuauDebug.What = ANSI_TO_TCHAR(ar.what);

	return LuauDebug;
}

TMap<FString, FLuauValue> ULuauState::LuauGetLocals(int32 Level)
{
	TMap<FString, FLuauValue> ReturnValue;

	return ReturnValue;
}

void ULuauState::Debug_Hook(lua_State* L, lua_Debug* ar)
{
	ULuauState* LuauState = ULuauState::GetFromExtraSpace(L);
	FLuauDebug LuauDebug;
	lua_getinfo(L, -1, "lSn", ar);
	LuauDebug.CurrentLine = ar->currentline;
	LuauDebug.Source = ANSI_TO_TCHAR(ar->source);
	LuauDebug.Name = ANSI_TO_TCHAR(ar->name);
	//LuauDebug.NameWhat = ANSI_TO_TCHAR(ar->namewhat);
	LuauDebug.What = ANSI_TO_TCHAR(ar->what);
}

int ULuauState::MetaTableFunctionUserData__eq(lua_State* L)
{
	ULuauState* LuauState = ULuauState::GetFromExtraSpace(L);

	FLuauUserData* UserData = (FLuauUserData*)lua_touserdata(L, 1);
	if (!UserData->Context.IsValid())
	{
		lua_pushboolean(L, 0);
		luaL_error(L, "invalid UObject for UserData %p", UserData);
	}

	FLuauUserData* UserData2 = (FLuauUserData*)lua_touserdata(L, 2);
	if (!UserData2->Context.IsValid())
	{
		lua_pushboolean(L, 0);
		luaL_error(L, "invalid UObject for UserData %p", UserData2);
	}

	if (UserData->Type == UserData2->Type && UserData->Context.Get() == UserData2->Context.Get())
	{
		if (UserData->Type == ELuauValueType::UFunction)
		{
			if (!UserData->Function.IsValid())
			{
				lua_pushboolean(L, 0);
				luaL_error(L, "invalid UFunction for UserData %p", UserData);
			}
			if (!UserData2->Function.IsValid())
			{
				lua_pushboolean(L, 0);
				luaL_error(L, "invalid UFunction for UserData %p", UserData2);
			}
			if (UserData->Function.Get() == UserData2->Function.Get())
			{
				lua_pushboolean(L, 1);
				return 1;
			}
		}
		else if (UserData->Type == ELuauValueType::UObject)
		{
			lua_pushboolean(L, 1);
			return 1;
		}
	}

	lua_pushboolean(L, 0);
	return 1;
}

int ULuauState::MetaTableFunctionUserData__gc(lua_State* L)
{
	ULuauState* LuauState = ULuauState::GetFromExtraSpace(L);

	FLuauUserData* UserData = (FLuauUserData*)lua_touserdata(L, 1);
	if (!UserData->Context.IsValid())
	{
		lua_pushnil(L);
		luaL_error(L, "invalid UObject for UserData %p", UserData);
	}

	ULuauUserDataObject* LuauUserDataObject = Cast<ULuauUserDataObject>(UserData->Context.Get());
	if (LuauUserDataObject)
	{
		LuauState->TrackedLuauUserDataObjects.Remove(LuauUserDataObject);
		LuauUserDataObject->ReceiveLuauGC();
	}

	lua_pushnil(L);
	return 1;
}

int ULuauState::MetaTableFunction__call(lua_State* L)
{
	ULuauState* LuauState = ULuauState::GetFromExtraSpace(L);
	FLuauUserData* LuauCallContext = (FLuauUserData*)lua_touserdata(L, 1);

	if (!LuauCallContext->Context.IsValid() || !LuauCallContext->Function.IsValid())
	{
		lua_pushnil(L);
		luaL_error(L, "invalid lua UFunction for UserData %p", LuauCallContext);
	}

	int NArgs = lua_gettop(L);

	UObject* CallScope = LuauCallContext->Context.Get();
	bool bImplicitSelf = false;
	int StackPointer = 2;

	if (ULuauComponent* LuauComponent = Cast<ULuauComponent>(CallScope))
	{
		CallScope = LuauComponent->GetOwner();
		if (NArgs > 0)
		{
			FLuauValue LuauFirstArgument = LuauState->ToLuauValue(StackPointer, L);
			if (LuauFirstArgument.Type == ELuauValueType::UObject && LuauFirstArgument.Object == LuauComponent)
			{
				bImplicitSelf = LuauComponent->bImplicitSelf;
			}
		}
	}
	else if (ULuauUserDataObject* LuauUserDataObject = Cast<ULuauUserDataObject>(CallScope))
	{
		if (NArgs > 0)
		{
			FLuauValue LuauFirstArgument = LuauState->ToLuauValue(StackPointer, L);
			if (LuauFirstArgument.Type == ELuauValueType::UObject && LuauFirstArgument.Object == LuauUserDataObject)
			{
				bImplicitSelf = LuauUserDataObject->bImplicitSelf;
			}
		}
	}

	FScopeCycleCounterUObject ObjectScope(CallScope);
	FScopeCycleCounterUObject FunctionScope(LuauCallContext->Function.Get());

	void* Parameters = FMemory_Alloca(LuauCallContext->Function->ParmsSize);
	FMemory::Memzero(Parameters, LuauCallContext->Function->ParmsSize);

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	for (TFieldIterator<FProperty> It(LuauCallContext->Function.Get()); (It && It->HasAnyPropertyFlags(CPF_Parm)); ++It)
	{
		FProperty* Prop = *It;
#else
	for (TFieldIterator<UProperty> It(LuauCallContext->Function.Get()); (It && It->HasAnyPropertyFlags(CPF_Parm)); ++It)
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
	for (TFieldIterator<FProperty> FArgs(LuauCallContext->Function.Get()); FArgs && ((FArgs->PropertyFlags & (CPF_Parm | CPF_ReturnParm)) == CPF_Parm); ++FArgs)
	{
		FProperty* Prop = *FArgs;
		FStructProperty* LuauProp = CastField<FStructProperty>(Prop);
#else
	for (TFieldIterator<UProperty> FArgs(LuauCallContext->Function.Get()); FArgs && ((FArgs->PropertyFlags & (CPF_Parm | CPF_ReturnParm)) == CPF_Parm); ++FArgs)
	{
		UProperty* Prop = *FArgs;
		UStructProperty* LuauProp = Cast<UStructProperty>(Prop);
#endif
		if (!LuauProp)
		{
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
			FArrayProperty* ArrayProp = CastField<FArrayProperty>(Prop);
			if (ArrayProp)
			{
				LuauProp = CastField<FStructProperty>(ArrayProp->Inner);
#else
			UArrayProperty* ArrayProp = Cast<UArrayProperty>(Prop);
			if (ArrayProp)
			{
				LuauProp = Cast<UStructProperty>(ArrayProp->Inner);
#endif
				if (!LuauProp)
				{
					break;
				}
				if (LuauProp->Struct != FLuauValue::StaticStruct())
				{
					break;
				}
				// start filling the array with the rest of arguments
				int ArgsToProcess = NArgs - StackPointer + 1;
				if (ArgsToProcess < 1)
				{
					break;
				}
				FScriptArrayHelper_InContainer ArrayHelper(ArrayProp, LuauProp->ContainerPtrToValuePtr<uint8>(Parameters));
				ArrayHelper.AddValues(ArgsToProcess);
				for (int i = StackPointer; i < StackPointer + ArgsToProcess; i++)
				{
					FLuauValue LuauValue = LuauState->ToLuauValue(i, L);
					*LuauProp->ContainerPtrToValuePtr<FLuauValue>(ArrayHelper.GetRawPtr(i - StackPointer)) = LuauValue;
				}
			}
			break;
		}
		if (LuauProp->Struct != FLuauValue::StaticStruct())
		{
			break;
		}

		FLuauValue LuauValue = LuauState->ToLuauValue(StackPointer++, L);
		*LuauProp->ContainerPtrToValuePtr<FLuauValue>(Parameters) = LuauValue;
	}

	LuauState->InceptionLevel++;
	CallScope->ProcessEvent(LuauCallContext->Function.Get(), Parameters);
	check(LuauState->InceptionLevel > 0);
	LuauState->InceptionLevel--;

	if (LuauState->InceptionLevel == 0)
	{
		FString Error;
		while (LuauState->InceptionErrors.Dequeue(Error))
		{
			ULuauComponent* LuauComponent = Cast<ULuauComponent>(LuauCallContext->Context);
			if (LuauComponent)
			{
				if (LuauComponent->bLogError)
				{
					LuauState->LogError(Error);
				}
				LuauComponent->OnLuauError.Broadcast(Error);
			}
			else
			{
				if (LuauState->bLogError)
				{
					LuauState->LogError(Error);
				}
				LuauState->ReceiveLuauError(Error);
			}
		}
	}

	int ReturnedValues = 0;

	// get return value
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	for (TFieldIterator<FProperty> FArgs(LuauCallContext->Function.Get()); FArgs; ++FArgs)
	{
		FProperty* Prop = *FArgs;
#else
	for (TFieldIterator<UProperty> FArgs(LuauCallContext->Function.Get()); FArgs; ++FArgs)
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
		FStructProperty* LuauProp = CastField<FStructProperty>(Prop);
		if (!LuauProp)
		{
			FArrayProperty* ArrayProp = CastField<FArrayProperty>(Prop);
			if (ArrayProp)
			{
				LuauProp = CastField<FStructProperty>(ArrayProp->Inner);
#else
		UStructProperty* LuauProp = Cast<UStructProperty>(Prop);
		if (!LuauProp)
		{
			UArrayProperty* ArrayProp = Cast<UArrayProperty>(Prop);
			if (ArrayProp)
			{
				LuauProp = Cast<UStructProperty>(ArrayProp->Inner);
#endif
				if (!LuauProp)
					break;
				if (LuauProp->Struct != FLuauValue::StaticStruct())
					break;

				FScriptArrayHelper_InContainer ArrayHelper(ArrayProp, LuauProp->ContainerPtrToValuePtr<uint8>(Parameters));
				for (int i = 0; i < ArrayHelper.Num(); i++)
				{
					FLuauValue* LuauValue = LuauProp->ContainerPtrToValuePtr<FLuauValue>(ArrayHelper.GetRawPtr(i));
					ReturnedValues++;
					LuauState->FromLuauValue(*LuauValue, nullptr, L);
				}


			}
			break;
		}

		if (LuauProp->Struct != FLuauValue::StaticStruct())
			break;

		FLuauValue* LuauValue = LuauProp->ContainerPtrToValuePtr<FLuauValue>(Parameters);
		if (LuauValue)
		{
			ReturnedValues++;
			LuauState->FromLuauValue(*LuauValue, nullptr, L);
		}
	}

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	for (TFieldIterator<FProperty> It(LuauCallContext->Function.Get()); (It && It->HasAnyPropertyFlags(CPF_Parm)); ++It)
#else
	for (TFieldIterator<UProperty> It(LuauCallContext->Function.Get()); (It && It->HasAnyPropertyFlags(CPF_Parm)); ++It)
#endif
	{
		It->DestroyValue_InContainer(Parameters);
	}


	if (ReturnedValues > 0)
		return ReturnedValues;

	lua_pushnil(L);
	return 1;
}

int ULuauState::MetaTableFunction__rawcall(lua_State * L)
{
	ULuauState* LuauState = ULuauState::GetFromExtraSpace(L);
	FLuauUserData* LuauCallContext = (FLuauUserData*)lua_touserdata(L, 1);

	if (!LuauCallContext->Context.IsValid() || !LuauCallContext->Function.IsValid())
	{
		lua_pushnil(L);
		luaL_error(L, "invalid lua UFunction for UserData %p", LuauCallContext);
		return 2;
	}

	int NArgs = lua_gettop(L);

	UObject* CallScope = LuauCallContext->Context.Get();
	bool bImplicitSelf = false;
	int StackPointer = 2;

	if (ULuauComponent* LuauComponent = Cast<ULuauComponent>(CallScope))
	{
		CallScope = LuauComponent->GetOwner();
		if (NArgs > 0)
		{
			FLuauValue LuauFirstArgument = LuauState->ToLuauValue(StackPointer, L);
			if (LuauFirstArgument.Type == ELuauValueType::UObject && LuauFirstArgument.Object == LuauComponent)
			{
				bImplicitSelf = LuauComponent->bImplicitSelf;
			}
		}
	}
	else if (ULuauUserDataObject* LuauUserDataObject = Cast<ULuauUserDataObject>(CallScope))
	{
		if (NArgs > 0)
		{
			FLuauValue LuauFirstArgument = LuauState->ToLuauValue(StackPointer, L);
			if (LuauFirstArgument.Type == ELuauValueType::UObject && LuauFirstArgument.Object == LuauUserDataObject)
			{
				bImplicitSelf = LuauUserDataObject->bImplicitSelf;
			}
		}
	}

	FScopeCycleCounterUObject ObjectScope(CallScope);
	FScopeCycleCounterUObject FunctionScope(LuauCallContext->Function.Get());

	void* Parameters = FMemory_Alloca(LuauCallContext->Function->ParmsSize);
	FMemory::Memzero(Parameters, LuauCallContext->Function->ParmsSize);

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	for (TFieldIterator<FProperty> It(LuauCallContext->Function.Get()); (It && It->HasAnyPropertyFlags(CPF_Parm)); ++It)
	{
		FProperty* Prop = *It;
#else
	for (TFieldIterator<UProperty> It(LuauCallContext->Function.Get()); (It && It->HasAnyPropertyFlags(CPF_Parm)); ++It)
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
	for (TFieldIterator<FProperty> FArgs(LuauCallContext->Function.Get()); FArgs && ((FArgs->PropertyFlags & (CPF_Parm | CPF_ReturnParm)) == CPF_Parm); ++FArgs)
	{
		FProperty* Prop = *FArgs;
#else
	for (TFieldIterator<UProperty> FArgs(LuauCallContext->Function.Get()); FArgs && ((FArgs->PropertyFlags & (CPF_Parm | CPF_ReturnParm)) == CPF_Parm); ++FArgs)
	{
		UProperty* Prop = *FArgs;
#endif
		bool bPropertySet = false;
		LuauState->ToProperty(Parameters, Prop, LuauState->ToLuauValue(StackPointer++, L), bPropertySet, 0);
	}

	LuauState->InceptionLevel++;
	CallScope->ProcessEvent(LuauCallContext->Function.Get(), Parameters);
	check(LuauState->InceptionLevel > 0);
	LuauState->InceptionLevel--;

	if (LuauState->InceptionLevel == 0)
	{
		FString Error;
		while (LuauState->InceptionErrors.Dequeue(Error))
		{
			ULuauComponent* LuauComponent = Cast<ULuauComponent>(LuauCallContext->Context);
			if (LuauComponent)
			{
				if (LuauComponent->bLogError)
				{
					LuauState->LogError(Error);
				}
				LuauComponent->OnLuauError.Broadcast(Error);
			}
			else
			{
				if (LuauState->bLogError)
				{
					LuauState->LogError(Error);
				}
				LuauState->ReceiveLuauError(Error);
			}
		}
	}

	int ReturnedValues = 0;

	// get return value
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	for (TFieldIterator<FProperty> FArgs(LuauCallContext->Function.Get()); FArgs; ++FArgs)
	{
		FProperty* Prop = *FArgs;
#else
	for (TFieldIterator<UProperty> FArgs(LuauCallContext->Function.Get()); FArgs; ++FArgs)
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
		FLuauValue LuauValue = LuauState->FromProperty(Parameters, Prop, bPropertyGet, 0);
		ReturnedValues++;
		LuauState->FromLuauValue(LuauValue, nullptr, L);
	}

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	for (TFieldIterator<FProperty> It(LuauCallContext->Function.Get()); (It && It->HasAnyPropertyFlags(CPF_Parm)); ++It)
#else
	for (TFieldIterator<UProperty> It(LuauCallContext->Function.Get()); (It && It->HasAnyPropertyFlags(CPF_Parm)); ++It)
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

int ULuauState::MetaTableFunction__rawbroadcast(lua_State * L)
{
	ULuauState* LuauState = ULuauState::GetFromExtraSpace(L);
	FLuauUserData* LuauCallContext = (FLuauUserData*)lua_touserdata(L, 1);

	if (!LuauCallContext->MulticastScriptDelegate || !LuauCallContext->Function.IsValid())
	{
		lua_pushnil(L);
		luaL_error(L, "invalid lua Multicast Delegate for UserData %p", LuauCallContext);
	}

	int NArgs = lua_gettop(L);
	int StackPointer = 2;

	FScopeCycleCounterUObject FunctionScope(LuauCallContext->Function.Get());

	void* Parameters = FMemory_Alloca(LuauCallContext->Function->ParmsSize);
	FMemory::Memzero(Parameters, LuauCallContext->Function->ParmsSize);

	for (TFieldIterator<FProperty> It(LuauCallContext->Function.Get()); (It && It->HasAnyPropertyFlags(CPF_Parm)); ++It)
	{
		FProperty* Prop = *It;
		if (!Prop->HasAnyPropertyFlags(CPF_ZeroConstructor))
		{
			Prop->InitializeValue_InContainer(Parameters);
		}
	}

	// arguments
	for (TFieldIterator<FProperty> FArgs(LuauCallContext->Function.Get()); FArgs && ((FArgs->PropertyFlags & (CPF_Parm | CPF_ReturnParm)) == CPF_Parm); ++FArgs)
	{
		FProperty* Prop = *FArgs;
		bool bPropertySet = false;
		LuauState->ToProperty(Parameters, Prop, LuauState->ToLuauValue(StackPointer++, L), bPropertySet, 0);
	}

	LuauState->InceptionLevel++;
	LuauCallContext->MulticastScriptDelegate->ProcessMulticastDelegate<UObject>(Parameters);
	check(LuauState->InceptionLevel > 0);
	LuauState->InceptionLevel--;

	if (LuauState->InceptionLevel == 0)
	{
		FString Error;
		while (LuauState->InceptionErrors.Dequeue(Error))
		{
			if (LuauState->bLogError)
			{
				LuauState->LogError(Error);
			}
			LuauState->ReceiveLuauError(Error);
		}
	}

	// no return values in multicast delegates
	for (TFieldIterator<FProperty> It(LuauCallContext->Function.Get()); (It && It->HasAnyPropertyFlags(CPF_Parm)); ++It)
	{
		It->DestroyValue_InContainer(Parameters);
	}

	lua_pushnil(L);
	return 1;
}

int ULuauState::TableFunction_print(lua_State * L)
{
	ULuauState* LuauState = ULuauState::GetFromExtraSpace(L);
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
		{
			luaL_error(L, "'tostring must return a string to 'print'");
			return 1;
		}
		FString Value = ANSI_TO_TCHAR(s);
		lua_pop(L, 1);
		Messages.Add(Value);
	}
	LuauState->Log(FString::Join(Messages, TEXT("\t")));
	return 0;
}

int ULuauState::TableFunction_package_loader_codeasset(lua_State * L)
{
	ULuauState* LuauState = ULuauState::GetFromExtraSpace(L);

	// use the second (sanitized by the loader) argument
	FString Key = ANSI_TO_TCHAR(lua_tostring(L, 2));

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION > 0
	FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(Key);
	if (AssetData.IsValid() && AssetData.AssetClassPath.ToString() == "LuauCode")
#else
	FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(*Key);
	if (AssetData.IsValid() && AssetData.AssetClass == "LuauCode")
#endif
	{
		ULuauCode* LuauCode = Cast<ULuauCode>(AssetData.GetAsset());
		if (LuauCode)
		{
			if (!LuauState->RunCodeAsset(LuauCode, 1))
			{
				luaL_error(L, "%s", lua_tostring(L, -1));
				return 2;
			}
			return 1;
		}
	}

	luaL_error(L, "unable to load asset '%s'", TCHAR_TO_UTF8(*Key));
	return 2;
}

int ULuauState::TableFunction_package_loader_asset(lua_State * L)
{
	ULuauState* LuauState = ULuauState::GetFromExtraSpace(L);

	// use the second (sanitized by the loader) argument
	const FString Key = ANSI_TO_TCHAR(lua_tostring(L, 2));

	if (LuauState->RunFile(Key, true, 1))
	{
		return 1;
	}
	luaL_error(L, "%s", lua_tostring(L, -1));
	return 1;
}

int ULuauState::TableFunction_package_loader(lua_State * L)
{
	ULuauState* LuauState = ULuauState::GetFromExtraSpace(L);

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

		lua_pushcfunction(L, ULuauState::TableFunction_package_loader_codeasset, "TableFunction_package_loader_codeasset");
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
			lua_pushcfunction(L, ULuauState::TableFunction_package_loader_asset, "TableFunction_package_loader_asset");
			lua_pushstring(L, TCHAR_TO_UTF8(*Key));
			return 2;
		}
		else
		{
			// or search in additional paths
			for (FString AdditionalPath : LuauState->AppendProjectContentDirSubDir)
			{
				AbsoluteFilename = FPaths::Combine(FPaths::ProjectContentDir(), AdditionalPath, Key);
				if (FPaths::FileExists(AbsoluteFilename))
				{
					lua_pushcfunction(L, ULuauState::TableFunction_package_loader_asset, "TableFunction_package_loader_asset");
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

int ULuauState::TableFunction_package_preload(lua_State * L)
{
	ULuauState* LuauState = ULuauState::GetFromExtraSpace(L);

	if (LuauState->L != L)
	{
		luaL_error(L, "you cannot call package.preload from a thread/coroutine (error while loading %s)", lua_tostring(L, 1));
		return 1;
	}

	FString Key = ANSI_TO_TCHAR(lua_tostring(L, 1));

	// check for code assets
	ULuauCode** LuauCodePtr = LuauState->RequireTable.Find(Key);
	if (!LuauCodePtr)
	{
		if (LuauState->bAddProjectContentDirToPackagePath && LuauState->RunFile(Key + ".lua", true, 1))
		{
			return 1;
		}

		// now search in additional paths
		for (FString AdditionalPath : LuauState->AppendProjectContentDirSubDir)
		{
			if (LuauState->RunFile(AdditionalPath / Key + ".lua", true, 1))
			{
				return 1;
			}
			luaL_error(L, "%s", lua_tostring(L, -1));
			return 1;

		}
		luaL_error(L, "unable to find package %s", TCHAR_TO_ANSI(*Key));
		return 1;
	}

	ULuauCode* LuauCode = *LuauCodePtr;
	if (!LuauCode)
	{
		luaL_error(L, "LuauCodeAsset not set for package %s", TCHAR_TO_ANSI(*Key));
		return 1;
	}

	if (!LuauState->RunCodeAsset(LuauCode, 1))
	{
		luaL_error(L, "%s", lua_tostring(L, -1));
		return 1;
	}

	return 1;
}

void ULuauState::ReceiveLuauError_Implementation(const FString & Message)
{

}

void ULuauState::ReceiveLuauCallHook_Implementation(const FLuauDebug & LuauDebug)
{

}

void ULuauState::ReceiveLuauReturnHook_Implementation(const FLuauDebug & LuauDebug)
{

}

void ULuauState::ReceiveLuauLineHook_Implementation(const FLuauDebug & LuauDebug)
{

}

void ULuauState::ReceiveLuauCountHook(const FLuauDebug & LuauDebug)
{

}

void ULuauState::ReceiveLuauLevelRemovedFromWorld_Implementation(ULevel * Level, UWorld * World)
{

}

void ULuauState::ReceiveLuauLevelAddedToWorld_Implementation(ULevel * Level, UWorld * World)
{

}

void ULuauState::ReceiveLuauStatePreInitialized_Implementation()
{

}

void ULuauState::ReceiveLuauStateInitialized_Implementation()
{

}

void ULuauState::NewTable()
{
	lua_newtable(L);
}

void ULuauState::SetMetaTable(int Index)
{
	lua_setmetatable(L, Index);
}

void ULuauState::GetMetaTable(int Index)
{
	lua_getmetatable(L, Index);
}

void ULuauState::SetField(int Index, const char* FieldName)
{
	lua_setfield(L, Index, FieldName);
}

void ULuauState::GetField(int Index, const char* FieldName)
{
	lua_getfield(L, Index, FieldName);
}

void ULuauState::RawGetI(int Index, int N)
{
	lua_rawgeti(L, Index, N);
}

void ULuauState::RawSetI(int Index, int N)
{
	lua_rawseti(L, Index, N);
}

void ULuauState::PushGlobalTable()
{
	lua_getglobal(L, "_G");
}

void ULuauState::PushRegistryTable()
{
	lua_pushvalue(L, LUA_REGISTRYINDEX);
}

int32 ULuauState::GetFieldFromTree(const FString & Tree, bool bGlobal)
{
	TArray<FString> Parts;
	Tree.ParseIntoArray(Parts, TEXT("."));
	if (Parts.Num() == 0)
	{
		LastError = FString::Printf(TEXT("invalid Luau key: \"%s\""), *Tree);
		if (bLogError)
			LogError(LastError);
		ReceiveLuauError(LastError);
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
			LastError = FString::Printf(TEXT("Luau key \"%s\" is nil"), *Parts[i]);
			if (bLogError)
				LogError(LastError);
			ReceiveLuauError(LastError);
			return i + 1 + AdditionalPop;
		}
	}
	return i + AdditionalPop;
}

void ULuauState::SetFieldFromTree(const FString & Tree, FLuauValue & Value, bool bGlobal, UObject * CallContext)
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
	FromLuauValue(Value, CallContext);
	SetField(-2, TCHAR_TO_ANSI(*Parts.Last()));
	Pop(ItemsToPop - 1);
}


void ULuauState::NewUObject(UObject * Object, lua_State * State)
{
	if (!State)
	{
		State = this->L;
	}
	FLuauUserData* UserData = (FLuauUserData*)lua_newuserdata(State, sizeof(FLuauUserData));
	UserData->Type = ELuauValueType::UObject;
	UserData->Context = Object;
	UserData->Function = nullptr;
}

void ULuauState::GetGlobal(const char* Name)
{
	lua_getglobal(L, Name);
}

void ULuauState::SetGlobal(const char* Name)
{
	lua_setglobal(L, Name);
}

void ULuauState::PushValue(int Index)
{
	lua_pushvalue(L, Index);
}

bool ULuauState::PCall(int NArgs, FLuauValue & Value, int NRet)
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
			ReceiveLuauError(LastError);
		}
	}
	return bSuccess;
}

bool ULuauState::Call(int NArgs, FLuauValue & Value, int NRet)
{
	if (lua_pcall(L, NArgs, NRet, 0))
	{
		LastError = FString::Printf(TEXT("Luau error: %s"), ANSI_TO_TCHAR(lua_tostring(L, -1)));
		return false;
	}

	if (NRet > 0)
	{
		Value = ToLuauValue(-1);
	}
	return true;
}

void ULuauState::Pop(int32 Amount)
{
	lua_pop(L, Amount);
}

void ULuauState::PushNil()
{
	lua_pushnil(L);
}

void ULuauState::PushCFunction(lua_CFunction Function)
{
	lua_pushcfunction(L, Function, "Function");
}

void* ULuauState::NewUserData(size_t DataSize)
{
	return lua_newuserdata(L, DataSize);
}

void ULuauState::Unref(int Ref)
{
	lua_unref(L, Ref);
}

void ULuauState::UnrefChecked(int Ref)
{
	// in case of moved value (like when compiling a blueprint), L should be nullptr
	if (!L)
		return;

	Unref(Ref);
}

int ULuauState::NewRef()
{
	return lua_ref(L, LUA_REGISTRYINDEX);
}

void ULuauState::GetRef(int Ref)
{
	lua_rawgeti(L, LUA_REGISTRYINDEX, Ref);
}

int ULuauState::Next(int Index)
{
	return lua_next(L, Index);
}

bool ULuauState::Yield(int Index, int NArgs)
{
	lua_State* Coroutine = lua_tothread(L, Index);
	if (!Coroutine)
		return false;

	if (lua_status(Coroutine) == LUA_OK && lua_gettop(Coroutine) == 0)
	{
		lua_pushboolean(L, 0);
		lua_pushstring(L, "Luau error: cannot yield a dead coroutine");
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

bool ULuauState::Resume(int Index, int NArgs)
{
	lua_State* Coroutine = lua_tothread(L, Index);
	if (!Coroutine)
		return false;

	if (lua_status(Coroutine) == LUA_OK && lua_gettop(Coroutine) == 0)
	{
		lua_pushboolean(L, 0);
		lua_pushstring(L, "Luau error: cannot resume dead coroutine");
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

int ULuauState::GC(int What, int Data)
{
	return lua_gc(L, What, Data);
}

void ULuauState::Len(int Index)
{
	(void)lua_objlen(L, Index);
}

int32 ULuauState::ILen(int Index)
{
	return lua_objlen(L, Index);
}

int32 ULuauState::ToInteger(int Index)
{
	return lua_tointeger(L, Index);
}

FLuauValue ULuauState::CreateLuauTable()
{
	FLuauValue NewTable;
	NewTable.Type = ELuauValueType::Table;
	NewTable.LuauState = this;
	FromLuauValue(NewTable);
	Pop();
	return NewTable;
}

FLuauValue ULuauState::CreateLuauLazyTable()
{
	FLuauValue NewTable;
	NewTable.Type = ELuauValueType::Table;
	NewTable.LuauState = this;
	return NewTable;
}

FLuauValue ULuauState::CreateLuauThread(FLuauValue Value)
{
	FLuauValue NewThread;
	NewThread.Type = ELuauValueType::Thread;
	NewThread.LuauState = this;
	FromLuauValue(NewThread);
	lua_State* NewLuauThread = lua_tothread(L, -1);
	FromLuauValue(Value, nullptr, NewLuauThread);
	Pop();
	return NewThread;
}

ELuauThreadStatus ULuauState::GetLuauThreadStatus(FLuauValue Value)
{
	if (Value.Type != ELuauValueType::Thread || Value.LuauState != this)
		return ELuauThreadStatus::Invalid;

	FromLuauValue(Value);
	lua_State* LuauThread = lua_tothread(L, -1);
	int ReturnValue = lua_status(LuauThread);
	Pop();

	if (ReturnValue == 0)
		return ELuauThreadStatus::Ok;

	if (ReturnValue == LUA_YIELD)
		return ELuauThreadStatus::Suspended;

	return ELuauThreadStatus::Error;
}

int32 ULuauState::GetLuauThreadStackTop(FLuauValue Value)
{
	if (Value.Type != ELuauValueType::Thread || Value.LuauState != this)
		return MIN_int32;

	FromLuauValue(Value);
	lua_State* LuauThread = lua_tothread(L, -1);
	int ReturnValue = lua_gettop(LuauThread);
	Pop();

	return ReturnValue;
}

TSharedRef<FLuauSmartReference> ULuauState::AddLuauSmartReference(FLuauValue Value)
{
	TSharedRef<FLuauSmartReference> Ref = MakeShared<FLuauSmartReference>();
	Ref->LuauState = this;
	Ref->Value = Value;

	LuauSmartReferences.Add(Ref);

	return Ref;
}

void ULuauState::RemoveLuauSmartReference(TSharedRef<FLuauSmartReference> Ref)
{
	LuauSmartReferences.Remove(Ref);
}

ULuauState::~ULuauState()
{
	FCoreUObjectDelegates::GetPostGarbageCollect().Remove(GCLuauDelegatesHandle);

#if WITH_EDITOR
	if (LuauConsole.LuauState)
	{
		IModularFeatures::Get().UnregisterModularFeature(IConsoleCommandExecutor::ModularFeatureName(), &LuauConsole);
	}
#endif

	FLuauMachineModule::Get().UnregisterLuauState(this);

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
		return FLuauValue((Type2)__##Type##__->GetPropertyValue_InContainer(Buffer, Index));\
	}

#define LUAVALUE_PROP_CAST_TOSTRING(Type) F##Type* __##Type##__ = CastField<F##Type>(Property);\
	if (__##Type##__)\
	{\
		return FLuauValue(__##Type##__->GetPropertyValue_InContainer(Buffer, Index).ToString());\
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
		return FLuauValue((Type2)__##Type##__->GetPropertyValue_InContainer(Buffer, Index));\
	}

#define LUAVALUE_PROP_CAST_TOSTRING(Type) U##Type* __##Type##__ = Cast<U##Type>(Property);\
	if (__##Type##__)\
	{\
		return FLuauValue(__##Type##__->GetPropertyValue_InContainer(Buffer, Index).ToString());\
	}

#define LUAVALUE_PROP_SET(Type, Value) U##Type* __##Type##__ = Cast<U##Type>(Property);\
	if (__##Type##__)\
	{\
		__##Type##__->SetPropertyValue_InContainer(Buffer, Value, Index);\
		return;\
	}
#endif

#if	ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
FLuauValue ULuauState::FromUProperty(void* Buffer, FProperty * Property, bool& bSuccess, int32 Index)
{
	return FromFProperty(Buffer, Property, bSuccess, Index);
}
void ULuauState::ToUProperty(void* Buffer, FProperty * Property, FLuauValue Value, bool& bSuccess, int32 Index)
{
	ToFProperty(Buffer, Property, Value, bSuccess, Index);
}
#endif

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
FLuauValue ULuauState::FromFProperty(void* Buffer, FProperty * Property, bool& bSuccess, int32 Index)
#else
FLuauValue ULuauState::FromUProperty(void* Buffer, UProperty * Property, bool& bSuccess, int32 Index)
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
		return FLuauValue((int32)*EnumValue);
	}
#endif

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	FObjectPropertyBase* ObjectPropertyBase = CastField<FObjectPropertyBase>(Property);
#else
	UObjectPropertyBase* ObjectPropertyBase = Cast<UObjectPropertyBase>(Property);
#endif
	if (ObjectPropertyBase)
	{
		return FLuauValue(ObjectPropertyBase->GetObjectPropertyValue_InContainer(Buffer, Index));
	}

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	FWeakObjectProperty* WeakObjectProperty = CastField<FWeakObjectProperty>(Property);
#else
	UWeakObjectProperty* WeakObjectProperty = Cast<UWeakObjectProperty>(Property);
#endif
	if (WeakObjectProperty)
	{
		const FWeakObjectPtr& WeakPtr = WeakObjectProperty->GetPropertyValue_InContainer(Buffer, Index);
		return FLuauValue(WeakPtr.Get());
	}

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	if (FMulticastDelegateProperty* MulticastProperty = CastField<FMulticastDelegateProperty>(Property))
#else
	if (UMulticastDelegateProperty* MulticastProperty = Cast<UMulticastDelegateProperty>(Property))
#endif
	{
		FLuauValue MulticastValue;
		MulticastValue.Type = ELuauValueType::MulticastDelegate;
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
		return FLuauValue::FunctionOfObject((UObject*)ScriptDelegate.GetUObject(), ScriptDelegate.GetFunctionName());
	}

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	if (FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
#else
	if (UArrayProperty* ArrayProperty = Cast<UArrayProperty>(Property))
#endif
	{
		FLuauValue NewLuauArray = CreateLuauTable();
		FScriptArrayHelper_InContainer Helper(ArrayProperty, Buffer, Index);
		for (int32 ArrayIndex = 0; ArrayIndex < Helper.Num(); ArrayIndex++)
		{
			uint8* ArrayItemPtr = Helper.GetRawPtr(ArrayIndex);
			bool bArrayItemSuccess = false;
			NewLuauArray.SetFieldByIndex(ArrayIndex + 1, FromProperty(ArrayItemPtr, ArrayProperty->Inner, bArrayItemSuccess, 0));
		}
		return NewLuauArray;
	}

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	if (FMapProperty* MapProperty = CastField<FMapProperty>(Property))
#else
	if (UMapProperty* MapProperty = Cast<UMapProperty>(Property))
#endif
	{
		FLuauValue NewLuauTable = CreateLuauTable();
		FScriptMapHelper_InContainer Helper(MapProperty, Buffer, Index);
		for (int32 MapIndex = 0; MapIndex < Helper.Num(); MapIndex++)
		{
			uint8* ArrayKeyPtr = Helper.GetKeyPtr(MapIndex);
			uint8* ArrayValuePtr = Helper.GetValuePtr(MapIndex);
			bool bArrayItemSuccess = false;
			NewLuauTable.SetField(
				FromProperty(ArrayKeyPtr, MapProperty->KeyProp, bArrayItemSuccess, 0).ToString(),
				FromProperty(ArrayValuePtr, MapProperty->ValueProp, bArrayItemSuccess, 0));
		}
		return NewLuauTable;
	}

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	if (FSetProperty* SetProperty = CastField<FSetProperty>(Property))
#else
	if (USetProperty* SetProperty = Cast<USetProperty>(Property))
#endif
	{
		FLuauValue NewLuauArray = CreateLuauTable();
		FScriptSetHelper_InContainer Helper(SetProperty, Buffer, Index);
		for (int32 SetIndex = 0; SetIndex < Helper.Num(); SetIndex++)
		{
			uint8* ArrayItemPtr = Helper.GetElementPtr(SetIndex);
			bool bArrayItemSuccess = false;
			NewLuauArray.SetFieldByIndex(SetIndex + 1, FromProperty(ArrayItemPtr, SetProperty->ElementProp, bArrayItemSuccess, 0));
		}
		return NewLuauArray;
	}

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
#else
	if (UStructProperty* StructProperty = Cast<UStructProperty>(Property))
#endif
	{
		// fast path
		if (StructProperty->Struct == FLuauValue::StaticStruct())
		{
			FLuauValue* LuauValuePtr = StructProperty->ContainerPtrToValuePtr<FLuauValue>(Buffer);
			// trick for allowing lazy tables creation
			FromLuauValue(*LuauValuePtr);
			Pop();
			return *LuauValuePtr;
		}

		const uint8* StructContainer = StructProperty->ContainerPtrToValuePtr<const uint8>(Buffer, Index);

		return StructToLuauTable(StructProperty->Struct, StructContainer);
	}

	bSuccess = false;
	return FLuauValue();
}

FLuauValue ULuauState::StructToLuauTable(UScriptStruct * InScriptStruct, const uint8 * StructData)
{
	FLuauValue NewLuauTable = CreateLuauTable();
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
		NewLuauTable.SetField(PropName, FromProperty((void*)StructData, FieldProp, bTableItemSuccess, 0));
	}
	return NewLuauTable;
}

FLuauValue ULuauState::StructToLuauTable(UScriptStruct * InScriptStruct, const TArray<uint8>&StructData)
{
	return StructToLuauTable(InScriptStruct, StructData.GetData());
}

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
void ULuauState::ToFProperty(void* Buffer, FProperty * Property, FLuauValue Value, bool& bSuccess, int32 Index)
#else
void ULuauState::ToUProperty(void* Buffer, UProperty * Property, FLuauValue Value, bool& bSuccess, int32 Index)
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
			UnregisterLuauDelegatesOfObject(Object);
			MulticastProperty->ClearDelegate(Object);
			return;
		}

		ULuauDelegate* LuauDelegate = NewObject<ULuauDelegate>();
		LuauDelegate->SetupLuauDelegate(MulticastProperty->SignatureFunction, this, Value);
		RegisterLuauDelegate((UObject*)Buffer, LuauDelegate);

		FScriptDelegate Delegate;
		Delegate.BindUFunction(LuauDelegate, FName("LuauDelegateFunction"));

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
			UnregisterLuauDelegatesOfObject(Object);
			DelegateProperty->SetPropertyValue_InContainer(Buffer, FScriptDelegate(), Index);
			return;
		}

		ULuauDelegate* LuauDelegate = NewObject<ULuauDelegate>();
		LuauDelegate->SetupLuauDelegate(DelegateProperty->SignatureFunction, this, Value);
		RegisterLuauDelegate((UObject*)Buffer, LuauDelegate);

		FScriptDelegate Delegate;
		Delegate.BindUFunction(LuauDelegate, FName("LuauDelegateFunction"));

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
		if (StructProperty->Struct == FLuauValue::StaticStruct())
		{
			FLuauValue* LuauValuePtr = StructProperty->ContainerPtrToValuePtr<FLuauValue>(Buffer);
			*LuauValuePtr = Value;
			return;
		}

		const uint8* StructContainer = StructProperty->ContainerPtrToValuePtr<const uint8>(Buffer, Index);
		LuauTableToStruct(Value, StructProperty->Struct, (uint8*)StructContainer);
		return;
	}

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
	if (FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
#else
	if (UArrayProperty* ArrayProperty = Cast<UArrayProperty>(Property))
#endif
	{
		FScriptArrayHelper_InContainer Helper(ArrayProperty, Buffer, Index);
		TArray<FLuauValue> ArrayValues = ULuauBlueprintFunctionLibrary::LuauTableGetValues(Value);
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
		TArray<FLuauValue> TableKeys = ULuauBlueprintFunctionLibrary::LuauTableGetKeys(Value);
		for (FLuauValue TableKey : TableKeys)
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
		TArray<FLuauValue> ArrayValues = ULuauBlueprintFunctionLibrary::LuauTableGetValues(Value);
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

void ULuauState::LuauTableToStruct(FLuauValue & LuauValue, UScriptStruct * InScriptStruct, uint8 * StructData)
{
	TArray<FLuauValue> TableKeys = ULuauBlueprintFunctionLibrary::LuauTableGetKeys(LuauValue);
	for (FLuauValue TableKey : TableKeys)
	{
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
		FProperty* StructProp = InScriptStruct->FindPropertyByName(TableKey.ToName());
#else
		UProperty* StructProp = InScriptStruct->FindPropertyByName(TableKey.ToName());
#endif
		if (StructProp)
		{
			bool bStructValueSuccess = false;
			ToProperty((void*)StructData, StructProp, LuauValue.GetField(TableKey.ToString()), bStructValueSuccess, 0);
		}
	}
}

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
void ULuauState::ToProperty(void* Buffer, FProperty * Property, FLuauValue Value, bool& bSuccess, int32 Index)
{
	ToFProperty(Buffer, Property, Value, bSuccess, Index);
}

FLuauValue ULuauState::FromProperty(void* Buffer, FProperty * Property, bool& bSuccess, int32 Index)
{
	return FromFProperty(Buffer, Property, bSuccess, Index);
}
#else
void ULuauState::ToProperty(void* Buffer, UProperty * Property, FLuauValue Value, bool& bSuccess, int32 Index)
{
	ToUProperty(Buffer, Property, Value, bSuccess, Index);
}
FLuauValue ULuauState::FromProperty(void* Buffer, UProperty * Property, bool& bSuccess, int32 Index)
{
	return FromUProperty(Buffer, Property, bSuccess, Index);
}
#endif


FLuauValue ULuauState::GetLuauValueFromProperty(UObject * InObject, const FString & PropertyName)
{
	if (!InObject)
	{
		return FLuauValue();
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

	return FLuauValue();
}

bool ULuauState::SetPropertyFromLuauValue(UObject * InObject, const FString & PropertyName, FLuauValue Value)
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

void ULuauState::SetUserDataMetaTable(FLuauValue MetaTable)
{
	UserDataMetaTable = MetaTable;
}

void ULuauState::SetupAndAssignUserDataMetatable(UObject * Context, TMap<FString, FLuauValue>&Metatable, lua_State * State)
{
	if (!State)
	{
		State = this->L;
	}

	lua_newtable(State);
	lua_pushcfunction(State, ULuauState::MetaTableFunctionUserData__index, "MetaTableFunctionUserData__index");
	lua_setfield(State, -2, "__index");
	lua_pushcfunction(State, ULuauState::MetaTableFunctionUserData__newindex, "MetaTableFunctionUserData__newindex");
	lua_setfield(State, -2, "__newindex");
	lua_pushcfunction(State, ULuauState::MetaTableFunctionUserData__eq, "MetaTableFunctionUserData__eq");
	lua_setfield(State, -2, "__eq");
	if (Context->IsA<ULuauUserDataObject>())
	{
		lua_pushcfunction(State, ULuauState::MetaTableFunctionUserData__gc, "MetaTableFunctionUserData__gc");
		lua_setfield(State, -2, "__gc");
	}

	for (TPair<FString, FLuauValue>& Pair : Metatable)
	{
		// first check for UFunction
		if (Pair.Value.Type == ELuauValueType::UFunction)
		{
			UObject* FunctionOwner = Context;
			if (ULuauComponent* LuauComponent = Cast<ULuauComponent>(Context))
			{
				FunctionOwner = LuauComponent->GetOwner();
			}

			if (FunctionOwner)
			{
				UFunction* Function = FunctionOwner->FindFunction(Pair.Value.FunctionName);
				if (Function)
				{
					FLuauUserData* LuauCallContext = (FLuauUserData*)lua_newuserdata(State, sizeof(FLuauUserData));
					LuauCallContext->Type = ELuauValueType::UFunction;
					LuauCallContext->Context = Context;
					LuauCallContext->Function = Function;

					lua_newtable(State);
					lua_pushcfunction(State, bRawLuauFunctionCall ? ULuauState::MetaTableFunction__rawcall : ULuauState::MetaTableFunction__call, bRawLuauFunctionCall ? "MetaTableFunction__rawcall" : "MetaTableFunction__call");
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
			FromLuauValue(Pair.Value, nullptr, State);
		}
		lua_setfield(State, -2, TCHAR_TO_ANSI(*Pair.Key));
	}

	lua_setmetatable(State, -2);
}

FLuauValue ULuauState::NewLuauUserDataObject(TSubclassOf<ULuauUserDataObject> LuauUserDataObjectClass, bool bTrackObject)
{
	ULuauUserDataObject* LuauUserDataObject = NewObject<ULuauUserDataObject>(this, LuauUserDataObjectClass);
	if (LuauUserDataObject)
	{
		if (bTrackObject)
		{
			TrackedLuauUserDataObjects.Add(LuauUserDataObject);
		}
		LuauUserDataObject->ReceiveLuauUserDataTableInit();
		return FLuauValue(LuauUserDataObject);
	}

	return FLuauValue();
}

void ULuauState::SetLuauUserDataField(FLuauValue UserData, const FString & Key, FLuauValue Value)
{
	if (UserData.Type != ELuauValueType::UObject || !UserData.Object)
		return;

	if (ULuauComponent* LuauComponent = Cast<ULuauComponent>(UserData.Object))
	{
		LuauComponent->LuauSetField(Key, Value);
		return;
	}

	if (ULuauUserDataObject* LuauUserDataObject = Cast<ULuauUserDataObject>(UserData.Object))
	{
		LuauUserDataObject->LuauSetField(Key, Value);
		return;
	}
}

FLuauValue ULuauState::GetLuauUserDataField(FLuauValue UserData, const FString & Key)
{
	if (UserData.Type != ELuauValueType::UObject || !UserData.Object)
		return FLuauValue();

	if (ULuauComponent* LuauComponent = Cast<ULuauComponent>(UserData.Object))
	{
		return LuauComponent->LuauGetField(Key);
	}

	if (ULuauUserDataObject* LuauUserDataObject = Cast<ULuauUserDataObject>(UserData.Object))
	{
		return LuauUserDataObject->LuauGetField(Key);
	}

	return FLuauValue();
}

const void* ULuauState::ToPointer(int Index)
{
	return lua_topointer(L, Index);
}

void ULuauState::LuauStateInit()
{
}

void ULuauState::GCLuauDelegatesCheck()
{
	TSet<TWeakObjectPtr<UObject>> DeadObjects;
	for (TPair<TWeakObjectPtr<UObject>, FLuauDelegateGroup>& Pair : LuauDelegatesMap)
	{
		if (!Pair.Key.IsValid())
		{
			DeadObjects.Add(Pair.Key);
		}
	}

	for (TWeakObjectPtr<UObject>& WeakObjectPtr : DeadObjects)
	{
		LuauDelegatesMap.Remove(WeakObjectPtr);
	}
}

void ULuauState::RegisterLuauDelegate(UObject * InObject, ULuauDelegate * InLuauDelegate)
{
	FLuauDelegateGroup* LuauDelegateGroup = LuauDelegatesMap.Find(InObject);
	if (LuauDelegateGroup)
	{
		LuauDelegateGroup->LuauDelegates.Add(InLuauDelegate);
	}
	else
	{
		FLuauDelegateGroup NewLuauDelegateGroup;
		NewLuauDelegateGroup.LuauDelegates.Add(InLuauDelegate);
		LuauDelegatesMap.Add(InObject, NewLuauDelegateGroup);
	}
}

void ULuauState::UnregisterLuauDelegatesOfObject(UObject* InObject)
{
	LuauDelegatesMap.Remove(InObject);
}

TArray<FString> ULuauState::GetPropertiesNames(UObject * InObject)
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

TArray<FString> ULuauState::GetFunctionsNames(UObject * InObject)
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

void ULuauState::AddLuauValueToLuauState(const FString & Name, FLuauValue LuauValue)
{
	SetFieldFromTree(Name, LuauValue, true);
}

FLuauValue ULuauState::RunString(const FString & CodeString, FString CodePath)
{
	FLuauValue ReturnValue;
	if (CodePath.IsEmpty())
	{
		CodePath = CodeString;
	}

	if (!RunCode(CodeString, CodePath, 1))
	{
		if (bLogError)
			LogError(LastError);
		ReceiveLuauError(LastError);
	}
	else
	{
		ReturnValue = ToLuauValue(-1);
	}

	Pop();
	return ReturnValue;
}

void ULuauState::Error(const FString& ErrorString)
{
	luaL_error(L, TCHAR_TO_UTF8(*ErrorString));
}