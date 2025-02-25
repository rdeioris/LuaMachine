// Copyright 2018-2023 - Roberto De Ioris

#include "LuaBlueprintFunctionLibrary.h"
#include "LuaComponent.h"
#include "LuaMachine.h"
#include "Runtime/Online/HTTP/Public/Interfaces/IHttpResponse.h"
#include "Runtime/Core/Public/Math/BigInt.h"
#include "Runtime/Core/Public/Misc/Base64.h"
#include "Runtime/Core/Public/Misc/SecureHash.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/Texture2D.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "IPlatformFilePak.h"
#if ENGINE_MAJOR_VERSION >= 5
#include "HAL/PlatformFileManager.h" 
#else
#include "HAL/PlatformFilemanager.h"
#endif
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION > 0
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetRegistry/AssetRegistryModule.h"
#else
#include "IAssetRegistry.h"
#include "AssetRegistryModule.h"
#endif
#include "Misc/FileHelper.h"
#include "Serialization/ArrayReader.h"
#include "TextureResource.h"

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 5
#include "Engine/BlueprintGeneratedClass.h"
#endif

FLuaValue ULuaBlueprintFunctionLibrary::LuaCreateNil()
{
	return FLuaValue();
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaCreateString(const FString& String)
{
	return FLuaValue(String);
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaCreateNumber(const float Value)
{
	return FLuaValue(Value);
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaCreateInteger(const int32 Value)
{
	return FLuaValue(Value);
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaCreateInteger64(const int64 Value)
{
	return FLuaValue(Value);
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaCreateBool(const bool bInBool)
{
	return FLuaValue(bInBool);
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaCreateObject(UObject* InObject)
{
	return FLuaValue(InObject);
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaCreateUFunction(UObject* InObject, const FString& FunctionName)
{
	if (InObject && InObject->FindFunction(FName(*FunctionName)))
	{
		FLuaValue Value = FLuaValue::Function(FName(*FunctionName));
		Value.Object = InObject;
		return Value;
	}

	return FLuaValue();
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaCreateTable(UObject* WorldContextObject, TSubclassOf<ULuaState> State)
{
	FLuaValue LuaValue;
	ULuaState* L = FLuaMachineModule::Get().GetLuaState(State, WorldContextObject->GetWorld());
	if (!L)
		return LuaValue;

	return L->CreateLuaTable();
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaCreateLazyTable(UObject* WorldContextObject, TSubclassOf<ULuaState> State)
{
	FLuaValue LuaValue;
	ULuaState* L = FLuaMachineModule::Get().GetLuaState(State, WorldContextObject->GetWorld());
	if (!L)
		return LuaValue;

	return L->CreateLuaLazyTable();
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaCreateThread(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FLuaValue Value)
{
	FLuaValue LuaValue;
	ULuaState* L = FLuaMachineModule::Get().GetLuaState(State, WorldContextObject->GetWorld());
	if (!L)
		return LuaValue;

	return L->CreateLuaThread(Value);
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaCreateObjectInState(UObject* WorldContextObject, TSubclassOf<ULuaState> State, UObject* InObject)
{
	FLuaValue LuaValue;
	if (!InObject)
		return LuaValue;
	ULuaState* L = FLuaMachineModule::Get().GetLuaState(State, WorldContextObject->GetWorld());
	if (!L)
		return LuaValue;

	LuaValue = FLuaValue(InObject);
	LuaValue.LuaState = L;
	return LuaValue;
}

void ULuaBlueprintFunctionLibrary::LuaStateDestroy(UObject* WorldContextObject, TSubclassOf<ULuaState> State)
{
	ULuaState* L = FLuaMachineModule::Get().GetLuaState(State, WorldContextObject->GetWorld());
	if (!L)
		return;
	FLuaMachineModule::Get().UnregisterLuaState(L);
}

void ULuaBlueprintFunctionLibrary::LuaStateReload(UObject* WorldContextObject, TSubclassOf<ULuaState> State)
{
	ULuaState* L = FLuaMachineModule::Get().GetLuaState(State, WorldContextObject->GetWorld());
	if (!L)
		return;
	FLuaMachineModule::Get().UnregisterLuaState(L);
	FLuaMachineModule::Get().GetLuaState(State, WorldContextObject->GetWorld());
}

FString ULuaBlueprintFunctionLibrary::Conv_LuaValueToString(const FLuaValue& Value)
{
	return Value.ToString();
}

FVector ULuaBlueprintFunctionLibrary::Conv_LuaValueToFVector(const FLuaValue& Value)
{
	return LuaTableToVector(Value);
}

FRotator ULuaBlueprintFunctionLibrary::Conv_LuaValueToFRotator(const FLuaValue& Value)
{
	return LuaTableToRotator(Value);
}

FName ULuaBlueprintFunctionLibrary::Conv_LuaValueToName(const FLuaValue& Value)
{
	return FName(*Value.ToString());
}

FText ULuaBlueprintFunctionLibrary::Conv_LuaValueToText(const FLuaValue& Value)
{
	return FText::FromString(Value.ToString());
}

UObject* ULuaBlueprintFunctionLibrary::Conv_LuaValueToObject(const FLuaValue& Value)
{
	if (Value.Type == ELuaValueType::UObject)
	{
		return Value.Object;
	}
	return nullptr;
}

UClass* ULuaBlueprintFunctionLibrary::Conv_LuaValueToClass(const FLuaValue& Value)
{
	if (Value.Type == ELuaValueType::UObject)
	{
		UClass* Class = Cast<UClass>(Value.Object);
		if (Class)
			return Class;
		UBlueprint* Blueprint = Cast<UBlueprint>(Value.Object);
		if (Blueprint)
			return Blueprint->GeneratedClass;
	}
	return nullptr;
}

FLuaValue ULuaBlueprintFunctionLibrary::Conv_ObjectToLuaValue(UObject* Object)
{
	return FLuaValue(Object);
}


FLuaValue ULuaBlueprintFunctionLibrary::Conv_FloatToLuaValue(const float Value)
{
	return FLuaValue(Value);
}

FLuaValue ULuaBlueprintFunctionLibrary::Conv_BoolToLuaValue(const bool Value)
{
	return FLuaValue(Value);
}

int32 ULuaBlueprintFunctionLibrary::Conv_LuaValueToInt(const FLuaValue& Value)
{
	return Value.ToInteger();
}

int64 ULuaBlueprintFunctionLibrary::Conv_LuaValueToInt64(const FLuaValue& Value)
{
	return Value.ToInteger();
}

float ULuaBlueprintFunctionLibrary::Conv_LuaValueToFloat(const FLuaValue& Value)
{
	return Value.ToFloat();
}

bool ULuaBlueprintFunctionLibrary::Conv_LuaValueToBool(const FLuaValue& Value)
{
	return Value.ToBool();
}

FLuaValue ULuaBlueprintFunctionLibrary::Conv_IntToLuaValue(const int32 Value)
{
	return FLuaValue(Value);
}

FLuaValue ULuaBlueprintFunctionLibrary::Conv_Int64ToLuaValue(const int64 Value)
{
	return FLuaValue(Value);
}

FLuaValue ULuaBlueprintFunctionLibrary::Conv_StringToLuaValue(const FString& Value)
{
	return FLuaValue(Value);
}

FLuaValue ULuaBlueprintFunctionLibrary::Conv_TextToLuaValue(const FText& Value)
{
	return FLuaValue(Value.ToString());
}

FLuaValue ULuaBlueprintFunctionLibrary::Conv_NameToLuaValue(const FName Value)
{
	return FLuaValue(Value.ToString());
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaGetGlobal(UObject* WorldContextObject, TSubclassOf<ULuaState> State, const FString& Name)
{
	ULuaState* L = FLuaMachineModule::Get().GetLuaState(State, WorldContextObject->GetWorld());
	if (!L)
		return FLuaValue();

	uint32 ItemsToPop = L->GetFieldFromTree(Name);
	FLuaValue ReturnValue = L->ToLuaValue(-1);
	L->Pop(ItemsToPop);
	return ReturnValue;
}

int64 ULuaBlueprintFunctionLibrary::LuaValueToPointer(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FLuaValue Value)
{
	ULuaState* L = FLuaMachineModule::Get().GetLuaState(State, WorldContextObject->GetWorld());
	if (!L)
		return 0;

	L->FromLuaValue(Value);
	const void* Ptr = L->ToPointer(-1);
	L->Pop();

	return (int64)Ptr;
}

FString ULuaBlueprintFunctionLibrary::LuaValueToHexPointer(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FLuaValue Value)
{
	int64 Ptr = LuaValueToPointer(WorldContextObject, State, Value);
	if (FGenericPlatformProperties::IsLittleEndian())
	{
		uint8 BEPtr[8] =
		{
			(uint8)((Ptr >> 56) & 0xff),
			(uint8)((Ptr >> 48) & 0xff),
			(uint8)((Ptr >> 40) & 0xff),
			(uint8)((Ptr >> 32) & 0xff),
			(uint8)((Ptr >> 24) & 0xff),
			(uint8)((Ptr >> 16) & 0xff),
			(uint8)((Ptr >> 8) & 0xff),
			(uint8)((Ptr) & 0xff),
		};
		return BytesToHex((const uint8*)BEPtr, sizeof(int64));
	}
	return BytesToHex((const uint8*)&Ptr, sizeof(int64));
}

FString ULuaBlueprintFunctionLibrary::LuaValueToBase64(const FLuaValue& Value)
{
	return Value.ToBase64();
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaValueFromBase64(const FString& Base64)
{
	return FLuaValue::FromBase64(Base64);
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaValueFromUTF16(const FString& String)
{
	TArray<uint8> Bytes;
	if (FGenericPlatformProperties::IsLittleEndian())
	{
		for (int32 Index = 0; Index < String.Len(); Index++)
		{
			uint16 UTF16Char = (uint16)String[Index];
			Bytes.Add((uint8)(UTF16Char & 0xFF));
			Bytes.Add((uint8)((UTF16Char >> 8) & 0xFF));
		}
	}
	else
	{
		for (int32 Index = 0; Index < String.Len(); Index++)
		{
			uint16 UTF16Char = (uint16)String[Index];
			Bytes.Add((uint8)((UTF16Char >> 8) & 0xFF));
			Bytes.Add((uint8)(UTF16Char & 0xFF));
		}
	}
	return FLuaValue(Bytes);
}

FString ULuaBlueprintFunctionLibrary::LuaValueToUTF16(const FLuaValue& Value)
{
	FString ReturnValue;
	TArray<uint8> Bytes = Value.ToBytes();
	if (Bytes.Num() % 2 != 0)
	{
		return ReturnValue;
	}

	if (FGenericPlatformProperties::IsLittleEndian())
	{
		for (int32 Index = 0; Index < Bytes.Num(); Index += 2)
		{
			uint16 UTF16Low = Bytes[Index];
			uint16 UTF16High = Bytes[Index + 1];
			ReturnValue.AppendChar((TCHAR)((UTF16High << 8) | UTF16Low));
		}
	}
	else
	{
		for (int32 Index = 0; Index < Bytes.Num(); Index += 2)
		{
			uint16 UTF16High = Bytes[Index];
			uint16 UTF16Low = Bytes[Index + 1];
			ReturnValue.AppendChar((TCHAR)((UTF16High << 8) | UTF16Low));
		}
	}
	return ReturnValue;
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaValueFromUTF8(const FString& String)
{
	FTCHARToUTF8 UTF8String(*String);
	return FLuaValue((const char*)UTF8String.Get(), UTF8String.Length());
}

FString ULuaBlueprintFunctionLibrary::LuaValueToUTF8(const FLuaValue& Value)
{
	FString ReturnValue;
	TArray<uint8> Bytes = Value.ToBytes();
	Bytes.Add(0);
	return FString(UTF8_TO_TCHAR(Bytes.GetData()));
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaValueFromUTF32(const FString& String)
{
#if ENGINE_MINOR_VERSION >= 25
	FTCHARToUTF32 UTF32String(*String);
	return FLuaValue((const char*)UTF32String.Get(), UTF32String.Length());
#else
	UE_LOG(LogLuaMachine, Error, TEXT("UTF32 not supported in this engine version"));
	return FLuaValue();
#endif
}

FString ULuaBlueprintFunctionLibrary::LuaValueToUTF32(const FLuaValue& Value)
{
#if ENGINE_MINOR_VERSION >= 25
	FString ReturnValue;
	TArray<uint8> Bytes = Value.ToBytes();
	Bytes.Add(0);
	Bytes.Add(0);
	Bytes.Add(0);
	Bytes.Add(0);
	return FString(FUTF32ToTCHAR((const UTF32CHAR*)Bytes.GetData(), Bytes.Num() / 4).Get());
#else
	UE_LOG(LogLuaMachine, Error, TEXT("UTF32 not supported in this engine version"));
	return FString("");
#endif
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaRunFile(UObject* WorldContextObject, TSubclassOf<ULuaState> State, const FString& Filename, const bool bIgnoreNonExistent)
{
	FLuaValue ReturnValue;

	ULuaState* L = FLuaMachineModule::Get().GetLuaState(State, WorldContextObject->GetWorld());
	if (!L)
		return ReturnValue;

	if (!L->RunFile(Filename, bIgnoreNonExistent, 1))
	{
		if (L->bLogError)
			L->LogError(L->LastError);
		L->ReceiveLuaError(L->LastError);
	}
	else
	{
		ReturnValue = L->ToLuaValue(-1);
	}

	L->Pop();
	return ReturnValue;
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaRunNonContentFile(UObject* WorldContextObject, TSubclassOf<ULuaState> State, const FString& Filename, const bool bIgnoreNonExistent)
{
	FLuaValue ReturnValue;

	ULuaState* L = FLuaMachineModule::Get().GetLuaState(State, WorldContextObject->GetWorld());
	if (!L)
		return ReturnValue;

	if (!L->RunFile(Filename, bIgnoreNonExistent, 1, true))
	{
		if (L->bLogError)
			L->LogError(L->LastError);
		L->ReceiveLuaError(L->LastError);
	}
	else
	{
		ReturnValue = L->ToLuaValue(-1);
	}

	L->Pop();
	return ReturnValue;
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaRunString(UObject* WorldContextObject, TSubclassOf<ULuaState> State, const FString& CodeString, FString CodePath)
{
	FLuaValue ReturnValue;

	ULuaState* L = FLuaMachineModule::Get().GetLuaState(State, WorldContextObject->GetWorld());
	if (!L)
	{
		return ReturnValue;
	}

	return L->RunString(CodeString, CodePath);
}

ELuaThreadStatus ULuaBlueprintFunctionLibrary::LuaThreadGetStatus(FLuaValue Value)
{
	if (Value.Type != ELuaValueType::Thread || !Value.LuaState.IsValid())
		return ELuaThreadStatus::Invalid;

	return Value.LuaState->GetLuaThreadStatus(Value);
}

int32 ULuaBlueprintFunctionLibrary::LuaThreadGetStackTop(FLuaValue Value)
{
	if (Value.Type != ELuaValueType::Thread || !Value.LuaState.IsValid())
	{
		return MIN_int32;
	}

	return Value.LuaState->GetLuaThreadStackTop(Value);
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaRunCodeAsset(UObject* WorldContextObject, TSubclassOf<ULuaState> State, ULuaCode* CodeAsset)
{
	FLuaValue ReturnValue;

	if (!CodeAsset)
		return ReturnValue;

	ULuaState* L = FLuaMachineModule::Get().GetLuaState(State, WorldContextObject->GetWorld());
	if (!L)
		return ReturnValue;

	if (!L->RunCodeAsset(CodeAsset, 1))
	{
		if (L->bLogError)
			L->LogError(L->LastError);
		L->ReceiveLuaError(L->LastError);
	}
	else {
		ReturnValue = L->ToLuaValue(-1);
	}
	L->Pop();
	return ReturnValue;
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaRunByteCode(UObject* WorldContextObject, TSubclassOf<ULuaState> State, const TArray<uint8>& ByteCode, const FString& CodePath)
{
	FLuaValue ReturnValue;

	ULuaState* L = FLuaMachineModule::Get().GetLuaState(State, WorldContextObject->GetWorld());
	if (!L)
		return ReturnValue;

	if (!L->RunCode(ByteCode, CodePath, 1))
	{
		if (L->bLogError)
			L->LogError(L->LastError);
		L->ReceiveLuaError(L->LastError);
	}
	else
	{
		ReturnValue = L->ToLuaValue(-1);
	}
	L->Pop();
	return ReturnValue;
}

UTexture2D* ULuaBlueprintFunctionLibrary::LuaValueToTransientTexture(int32 Width, int32 Height, const FLuaValue& Value, EPixelFormat PixelFormat, bool bDetectFormat)
{
	if (Value.Type != ELuaValueType::String)
	{
		return nullptr;
	}

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));

	TArray<uint8> Bytes = Value.ToBytes();

	if (bDetectFormat)
	{
		EImageFormat ImageFormat = ImageWrapperModule.DetectImageFormat(Bytes.GetData(), Bytes.Num());
		if (ImageFormat == EImageFormat::Invalid)
		{
			UE_LOG(LogLuaMachine, Error, TEXT("Unable to detect image format"));
			return nullptr;
		}

		TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(ImageFormat);
		if (!ImageWrapper.IsValid())
		{
			UE_LOG(LogLuaMachine, Error, TEXT("Unable to create ImageWrapper"));
			return nullptr;
		}

		if (!ImageWrapper->SetCompressed(Bytes.GetData(), Bytes.Num()))
		{
			UE_LOG(LogLuaMachine, Error, TEXT("Unable to parse image data"));
			return nullptr;
		}

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
		TArray<uint8> UncompressedBytes;
#else
		const TArray<uint8>* UncompressedBytes = nullptr;
#endif
		if (!ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedBytes))
		{
			UE_LOG(LogLuaMachine, Error, TEXT("Unable to get raw image data"));
			return nullptr;
		}
		PixelFormat = EPixelFormat::PF_B8G8R8A8;
		Width = ImageWrapper->GetWidth();
		Height = ImageWrapper->GetHeight();
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
		Bytes = UncompressedBytes;
#else
		Bytes = *UncompressedBytes;
#endif
	}

	UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, PixelFormat);
	if (!Texture)
	{
		return nullptr;
	}

#if ENGINE_MAJOR_VERSION > 4
	FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
#else
	FTexture2DMipMap& Mip = Texture->PlatformData->Mips[0];
#endif
	void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(Data, Bytes.GetData(), Bytes.Num());
	Mip.BulkData.Unlock();
	Texture->UpdateResource();

	return Texture;
}

void ULuaBlueprintFunctionLibrary::LuaHttpRequest(UObject* WorldContextObject, TSubclassOf<ULuaState> State, const FString& Method, const FString& URL, TMap<FString, FString> Headers, FLuaValue Body, FLuaValue Context, const FLuaHttpResponseReceived& ResponseReceived, const FLuaHttpError& Error)
{
	ULuaState* L = FLuaMachineModule::Get().GetLuaState(State, WorldContextObject->GetWorld());
	if (!L)
		return;

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 26
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
#else
	TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
#endif
	HttpRequest->SetVerb(Method);
	HttpRequest->SetURL(URL);
	for (TPair<FString, FString> Header : Headers)
	{
		HttpRequest->AppendToHeader(Header.Key, Header.Value);
	}
	HttpRequest->SetContent(Body.ToBytes());

	TSharedRef<FLuaSmartReference> ContextSmartRef = L->AddLuaSmartReference(Context);

	HttpRequest->OnProcessRequestComplete().BindStatic(&ULuaBlueprintFunctionLibrary::HttpGenericRequestDone, TWeakPtr<FLuaSmartReference>(ContextSmartRef), ResponseReceived, Error);
	HttpRequest->ProcessRequest();
}

void ULuaBlueprintFunctionLibrary::HttpGenericRequestDone(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, TWeakPtr<FLuaSmartReference> Context, FLuaHttpResponseReceived ResponseReceived, FLuaHttpError Error)
{
	// if the context is invalid, the LuaState is already dead
	if (!Context.IsValid())
		return;

	TSharedRef<FLuaSmartReference> SmartContext = Context.Pin().ToSharedRef();

	SmartContext->LuaState->RemoveLuaSmartReference(SmartContext);

	if (!bWasSuccessful)
	{
		Error.ExecuteIfBound(SmartContext->Value);
		return;
	}

	FLuaValue StatusCode = FLuaValue(Response->GetResponseCode());
	FLuaValue Headers = SmartContext->LuaState->CreateLuaTable();
	for (auto HeaderLine : Response->GetAllHeaders())
	{
		int32 Index;
		if (HeaderLine.Len() > 2 && HeaderLine.FindChar(':', Index))
		{
			FString Key;
			if (Index > 0)
				Key = HeaderLine.Left(Index);
			FString Value = HeaderLine.Right(HeaderLine.Len() - (Index + 2));
			Headers.SetField(Key, FLuaValue(Value));
		}
	}
	FLuaValue Content = FLuaValue(Response->GetContent());
	FLuaValue LuaHttpResponse = SmartContext->LuaState->CreateLuaTable();
	LuaHttpResponse.SetFieldByIndex(1, StatusCode);
	LuaHttpResponse.SetFieldByIndex(2, Headers);
	LuaHttpResponse.SetFieldByIndex(3, Content);
	ResponseReceived.ExecuteIfBound(SmartContext->Value, LuaHttpResponse);
}

void ULuaBlueprintFunctionLibrary::LuaRunURL(UObject* WorldContextObject, TSubclassOf<ULuaState> State, const FString& URL, TMap<FString, FString> Headers, const FString& SecurityHeader, const FString& SignaturePublicExponent, const FString& SignatureModulus, FLuaHttpSuccess Completed)
{

	// Security CHECK
	if (!SecurityHeader.IsEmpty() || !SignaturePublicExponent.IsEmpty() || !SignatureModulus.IsEmpty())
	{
		if (SecurityHeader.IsEmpty() || SignaturePublicExponent.IsEmpty() || SignatureModulus.IsEmpty())
		{
			UE_LOG(LogLuaMachine, Error, TEXT("For secure LuaRunURL() you need to specify the Security HTTP Header, the Signature Public Exponent and the Signature Modulus"));
			return;
		}
	}
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 26
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
#else
	TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
#endif
	HttpRequest->SetURL(URL);
	for (TPair<FString, FString> Header : Headers)
	{
		HttpRequest->AppendToHeader(Header.Key, Header.Value);
	}
	HttpRequest->OnProcessRequestComplete().BindStatic(&ULuaBlueprintFunctionLibrary::HttpRequestDone, State, TWeakObjectPtr<UWorld>(WorldContextObject->GetWorld()), SecurityHeader, SignaturePublicExponent, SignatureModulus, Completed);
	HttpRequest->ProcessRequest();
}

void ULuaBlueprintFunctionLibrary::HttpRequestDone(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, TSubclassOf<ULuaState> LuaState, TWeakObjectPtr<UWorld> World, const FString SecurityHeader, const FString SignaturePublicExponent, const FString SignatureModulus, FLuaHttpSuccess Completed)
{
	FLuaValue ReturnValue;
	int32 StatusCode = -1;

	if (!bWasSuccessful)
	{
		UE_LOG(LogLuaMachine, Error, TEXT("HTTP session failed for \"%s %s\""), *Request->GetVerb(), *Request->GetURL());
	}

	else if (!World.IsValid())
	{
		UE_LOG(LogLuaMachine, Error, TEXT("Unable to access LuaState as the World object is no more available (\"%s %s\")"), *Request->GetVerb(), *Request->GetURL());
	}
	else
	{
		// Check signature
		if (!SecurityHeader.IsEmpty())
		{
			// check code size
			if (Response->GetContentLength() <= 0)
			{
				UE_LOG(LogLuaMachine, Error, TEXT("[Security] Invalid Content Size"));
				Completed.ExecuteIfBound(ReturnValue, bWasSuccessful, StatusCode);
				return;
			}
			FString EncryptesSignatureBase64 = Response->GetHeader(SecurityHeader);
			if (EncryptesSignatureBase64.IsEmpty())
			{
				UE_LOG(LogLuaMachine, Error, TEXT("[Security] Invalid Security HTTP Header"));
				Completed.ExecuteIfBound(ReturnValue, bWasSuccessful, StatusCode);
				return;
			}

			const uint32 KeySize = 64;

			TArray<uint8> Signature;
			FBase64::Decode(EncryptesSignatureBase64, Signature);

			if (Signature.Num() != KeySize)
			{
				UE_LOG(LogLuaMachine, Error, TEXT("[Security] Invalid Signature"));
				Completed.ExecuteIfBound(ReturnValue, bWasSuccessful, StatusCode);
				return;
			}
			TEncryptionInt SignatureValue = TEncryptionInt((uint32*)&Signature[0]);

			TArray<uint8> PublicExponent;
			FBase64::Decode(SignaturePublicExponent, PublicExponent);
			if (PublicExponent.Num() != KeySize)
			{
				UE_LOG(LogLuaMachine, Error, TEXT("[Security] Invalid Signature Public Exponent"));
				Completed.ExecuteIfBound(ReturnValue, bWasSuccessful, StatusCode);
				return;
			}

			TArray<uint8> Modulus;
			FBase64::Decode(SignatureModulus, Modulus);
			if (Modulus.Num() != KeySize)
			{
				UE_LOG(LogLuaMachine, Error, TEXT("[Security] Invalid Signature Modulus"));
				Completed.ExecuteIfBound(ReturnValue, bWasSuccessful, StatusCode);
				return;
			}

			TEncryptionInt PublicKey = TEncryptionInt((uint32*)&PublicExponent[0]);
			TEncryptionInt ModulusValue = TEncryptionInt((uint32*)&Modulus[0]);

			TEncryptionInt ShaHash;
			FSHA1::HashBuffer(Response->GetContent().GetData(), Response->GetContentLength(), (uint8*)&ShaHash);

			TEncryptionInt DecryptedSignature = FEncryption::ModularPow(SignatureValue, PublicKey, ModulusValue);
			if (DecryptedSignature != ShaHash)
			{
				UE_LOG(LogLuaMachine, Error, TEXT("[Security] Signature check failed"));
				Completed.ExecuteIfBound(ReturnValue, bWasSuccessful, StatusCode);
				return;
			}
		}

		ReturnValue = LuaRunString(World.Get(), LuaState, Response->GetContentAsString());
		StatusCode = Response->GetResponseCode();
	}

	Completed.ExecuteIfBound(ReturnValue, bWasSuccessful, StatusCode);
}

void ULuaBlueprintFunctionLibrary::LuaTableFillObject(FLuaValue InTable, UObject* InObject)
{
	if (InTable.Type != ELuaValueType::Table || !InObject)
		return;

	ULuaState* L = InTable.LuaState.Get();
	if (!L)
		return;

	UStruct* Class = Cast<UStruct>(InObject);
	if (!Class)
		Class = InObject->GetClass();

	L->FromLuaValue(InTable);
	L->PushNil(); // first key
	while (L->Next(-2))
	{
		FLuaValue Key = L->ToLuaValue(-2);
		FLuaValue Value = L->ToLuaValue(-1);
		L->SetPropertyFromLuaValue(InObject, Key.ToString(), Value);
		L->Pop(); // pop the value
	}

	L->Pop(); // pop the table
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaTableGetField(FLuaValue Table, const FString& Key)
{
	ULuaState* L = Table.LuaState.Get();
	if (!L)
		return FLuaValue();

	return Table.GetField(Key);
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaComponentGetField(FLuaValue LuaComponent, const FString& Key)
{
	FLuaValue ReturnValue;
	if (LuaComponent.Type != ELuaValueType::UObject)
		return ReturnValue;

	ULuaState* L = LuaComponent.LuaState.Get();
	if (!L)
		return ReturnValue;

	ULuaComponent* Component = Cast<ULuaComponent>(LuaComponent.Object);

	FLuaValue* LuaValue = Component->Table.Find(Key);
	if (LuaValue)
	{
		return *LuaValue;
	}

	return ReturnValue;
}

bool ULuaBlueprintFunctionLibrary::LuaValueIsNil(const FLuaValue& Value)
{
	return Value.Type == ELuaValueType::Nil;
}

bool ULuaBlueprintFunctionLibrary::LuaValueIsOwned(const FLuaValue& Value)
{
	return Value.LuaState != nullptr;
}

TSubclassOf<ULuaState> ULuaBlueprintFunctionLibrary::LuaValueGetOwner(const FLuaValue& Value)
{
	if (!Value.LuaState.IsValid())
	{
		return nullptr;
	}
	return Value.LuaState->GetClass();
}

bool ULuaBlueprintFunctionLibrary::LuaValueIsNotNil(const FLuaValue& Value)
{
	return Value.Type != ELuaValueType::Nil;
}

bool ULuaBlueprintFunctionLibrary::LuaValueIsTable(const FLuaValue& Value)
{
	return Value.Type == ELuaValueType::Table;
}

bool ULuaBlueprintFunctionLibrary::LuaValueIsBoolean(const FLuaValue& Value)
{
	return Value.Type == ELuaValueType::Bool;
}

bool ULuaBlueprintFunctionLibrary::LuaValueIsThread(const FLuaValue& Value)
{
	return Value.Type == ELuaValueType::Thread;
}

bool ULuaBlueprintFunctionLibrary::LuaValueIsFunction(const FLuaValue& Value)
{
	return Value.Type == ELuaValueType::Function;
}

bool ULuaBlueprintFunctionLibrary::LuaValueIsNumber(const FLuaValue& Value)
{
	return Value.Type == ELuaValueType::Number;
}

bool ULuaBlueprintFunctionLibrary::LuaValueIsInteger(const FLuaValue& Value)
{
	return Value.Type == ELuaValueType::Integer;
}

bool ULuaBlueprintFunctionLibrary::LuaValueIsString(const FLuaValue& Value)
{
	return Value.Type == ELuaValueType::String;
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaTableGetByIndex(FLuaValue Table, int32 Index)
{
	if (Table.Type != ELuaValueType::Table)
		return FLuaValue();

	ULuaState* L = Table.LuaState.Get();
	if (!L)
		return FLuaValue();

	return Table.GetFieldByIndex(Index);
}

FLuaValue ULuaBlueprintFunctionLibrary::AssignLuaValueToLuaState(UObject* WorldContextObject, FLuaValue Value, TSubclassOf<ULuaState> State)
{
	ULuaState* L = FLuaMachineModule::Get().GetLuaState(State, WorldContextObject->GetWorld());
	if (!L)
		return Value;
	Value.LuaState = L;
	return Value;
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaTableSetByIndex(FLuaValue Table, int32 Index, FLuaValue Value)
{
	if (Table.Type != ELuaValueType::Table)
		return FLuaValue();

	ULuaState* L = Table.LuaState.Get();
	if (!L)
		return FLuaValue();

	return Table.SetFieldByIndex(Index, Value);
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaTableSetField(FLuaValue Table, const FString& Key, FLuaValue Value)
{
	FLuaValue ReturnValue;
	if (Table.Type != ELuaValueType::Table)
		return ReturnValue;

	ULuaState* L = Table.LuaState.Get();
	if (!L)
		return ReturnValue;

	return Table.SetField(Key, Value);
}

FLuaValue ULuaBlueprintFunctionLibrary::GetLuaComponentAsLuaValue(AActor* Actor)
{
	if (!Actor)
		return FLuaValue();

	return FLuaValue(Actor->GetComponentByClass(ULuaComponent::StaticClass()));
}

FLuaValue ULuaBlueprintFunctionLibrary::GetLuaComponentByStateAsLuaValue(AActor* Actor, TSubclassOf<ULuaState> State)
{
	if (!Actor)
		return FLuaValue();
#if ENGINE_MAJOR_VERSION < 5 && ENGINE_MINOR_VERSION < 24
	TArray<UActorComponent*> Components = Actor->GetComponentsByClass(ULuaComponent::StaticClass());
#else
	TArray<UActorComponent*> Components;
	Actor->GetComponents(Components);
#endif
	for (UActorComponent* Component : Components)
	{
		ULuaComponent* LuaComponent = Cast<ULuaComponent>(Component);
		if (LuaComponent)
		{
			if (LuaComponent->LuaState == State)
			{
				return FLuaValue(LuaComponent);
			}
		}
	}

	return FLuaValue();
}

FLuaValue ULuaBlueprintFunctionLibrary::GetLuaComponentByNameAsLuaValue(AActor* Actor, const FString& Name)
{
	if (!Actor)
		return FLuaValue();

#if ENGINE_MAJOR_VERSION < 5 && ENGINE_MINOR_VERSION < 24
	TArray<UActorComponent*> Components = Actor->GetComponentsByClass(ULuaComponent::StaticClass());
#else
	TArray<UActorComponent*> Components;
	Actor->GetComponents(Components);
#endif
	for (UActorComponent* Component : Components)
	{
		ULuaComponent* LuaComponent = Cast<ULuaComponent>(Component);
		if (LuaComponent)
		{
			if (LuaComponent->GetName() == Name)
			{
				return FLuaValue(LuaComponent);
			}
		}
	}

	return FLuaValue();
}

FLuaValue ULuaBlueprintFunctionLibrary::GetLuaComponentByStateAndNameAsLuaValue(AActor* Actor, TSubclassOf<ULuaState> State, const FString& Name)
{
	if (!Actor)
		return FLuaValue();

#if ENGINE_MAJOR_VERSION < 5 && ENGINE_MINOR_VERSION < 24
	TArray<UActorComponent*> Components = Actor->GetComponentsByClass(ULuaComponent::StaticClass());
#else
	TArray<UActorComponent*> Components;
	Actor->GetComponents(Components);
#endif
	for (UActorComponent* Component : Components)
	{
		ULuaComponent* LuaComponent = Cast<ULuaComponent>(Component);
		if (LuaComponent)
		{
			if (LuaComponent->LuaState == State && LuaComponent->GetName() == Name)
			{
				return FLuaValue(LuaComponent);
			}
		}
	}

	return FLuaValue();
}

int32 ULuaBlueprintFunctionLibrary::LuaGetTop(UObject* WorldContextObject, TSubclassOf<ULuaState> State)
{
	ULuaState* L = FLuaMachineModule::Get().GetLuaState(State, WorldContextObject->GetWorld());
	if (!L)
		return MIN_int32;
	return L->GetTop();
}

void ULuaBlueprintFunctionLibrary::LuaSetGlobal(UObject* WorldContextObject, TSubclassOf<ULuaState> State, const FString& Name, FLuaValue Value)
{
	ULuaState* L = FLuaMachineModule::Get().GetLuaState(State, WorldContextObject->GetWorld());
	if (!L)
		return;
	L->SetFieldFromTree(Name, Value, true);
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaGlobalCall(UObject* WorldContextObject, TSubclassOf<ULuaState> State, const FString& Name, TArray<FLuaValue> Args)
{
	FLuaValue ReturnValue;
	ULuaState* L = FLuaMachineModule::Get().GetLuaState(State, WorldContextObject->GetWorld());
	if (!L)
		return ReturnValue;

	int32 ItemsToPop = L->GetFieldFromTree(Name);

	int NArgs = 0;
	for (FLuaValue& Arg : Args)
	{
		L->FromLuaValue(Arg);
		NArgs++;
	}

	L->PCall(NArgs, ReturnValue);

	// we have the return value and the function has been removed, so we do not need to change ItemsToPop
	L->Pop(ItemsToPop);

	return ReturnValue;
}

TArray<FLuaValue> ULuaBlueprintFunctionLibrary::LuaGlobalCallMulti(UObject* WorldContextObject, TSubclassOf<ULuaState> State, const FString& Name, TArray<FLuaValue> Args)
{
	TArray<FLuaValue> ReturnValue;
	ULuaState* L = FLuaMachineModule::Get().GetLuaState(State, WorldContextObject->GetWorld());
	if (!L)
		return ReturnValue;

	int32 ItemsToPop = L->GetFieldFromTree(Name);

	int32 StackTop = L->GetTop();

	int NArgs = 0;
	for (FLuaValue& Arg : Args)
	{
		L->FromLuaValue(Arg);
		NArgs++;
	}

	FLuaValue LastReturnValue;
	if (L->PCall(NArgs, LastReturnValue, LUA_MULTRET))
	{

		int32 NumOfReturnValues = (L->GetTop() - StackTop) + 1;
		if (NumOfReturnValues > 0)
		{
			for (int32 i = -1; i >= -(NumOfReturnValues); i--)
			{
				ReturnValue.Insert(L->ToLuaValue(i), 0);
			}
			L->Pop(NumOfReturnValues - 1);
		}

	}

	// we have the return value and the function has been removed, so we do not need to change ItemsToPop
	L->Pop(ItemsToPop);

	return ReturnValue;
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaGlobalCallValue(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FLuaValue Value, TArray<FLuaValue> Args)
{
	FLuaValue ReturnValue;
	ULuaState* L = FLuaMachineModule::Get().GetLuaState(State, WorldContextObject->GetWorld());
	if (!L)
		return ReturnValue;

	L->FromLuaValue(Value);

	int NArgs = 0;
	for (FLuaValue& Arg : Args)
	{
		L->FromLuaValue(Arg);
		NArgs++;
	}

	L->PCall(NArgs, ReturnValue);

	L->Pop();

	return ReturnValue;
}

TArray<FLuaValue> ULuaBlueprintFunctionLibrary::LuaGlobalCallValueMulti(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FLuaValue Value, TArray<FLuaValue> Args)
{
	TArray<FLuaValue> ReturnValue;
	ULuaState* L = FLuaMachineModule::Get().GetLuaState(State, WorldContextObject->GetWorld());
	if (!L)
		return ReturnValue;

	L->FromLuaValue(Value);

	int32 StackTop = L->GetTop();

	int NArgs = 0;
	for (FLuaValue& Arg : Args)
	{
		L->FromLuaValue(Arg);
		NArgs++;
	}

	FLuaValue LastReturnValue;
	if (L->PCall(NArgs, LastReturnValue, LUA_MULTRET))
	{

		int32 NumOfReturnValues = (L->GetTop() - StackTop) + 1;
		if (NumOfReturnValues > 0)
		{
			for (int32 i = -1; i >= -(NumOfReturnValues); i--)
			{
				ReturnValue.Insert(L->ToLuaValue(i), 0);
			}
			L->Pop(NumOfReturnValues - 1);
		}

	}

	L->Pop();

	return ReturnValue;
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaValueCall(FLuaValue Value, TArray<FLuaValue> Args)
{
	FLuaValue ReturnValue;

	ULuaState* L = Value.LuaState.Get();
	if (!L)
		return ReturnValue;

	L->FromLuaValue(Value);

	int NArgs = 0;
	for (FLuaValue& Arg : Args)
	{
		L->FromLuaValue(Arg);
		NArgs++;
	}

	L->PCall(NArgs, ReturnValue);

	L->Pop();

	return ReturnValue;
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaValueCallIfNotNil(FLuaValue Value, TArray<FLuaValue> Args)
{
	FLuaValue ReturnValue;
	if (Value.Type != ELuaValueType::Nil)
		ReturnValue = LuaValueCall(Value, Args);

	return ReturnValue;
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaTableKeyCall(FLuaValue InTable, const FString& Key, TArray<FLuaValue> Args)
{
	FLuaValue ReturnValue;
	if (InTable.Type != ELuaValueType::Table)
		return ReturnValue;

	ULuaState* L = InTable.LuaState.Get();
	if (!L)
		return ReturnValue;

	FLuaValue Value = InTable.GetField(Key);
	if (Value.Type == ELuaValueType::Nil)
		return ReturnValue;

	return LuaValueCall(Value, Args);
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaTableKeyCallWithSelf(FLuaValue InTable, const FString& Key, TArray<FLuaValue> Args)
{
	FLuaValue ReturnValue;
	if (InTable.Type != ELuaValueType::Table)
		return ReturnValue;

	ULuaState* L = InTable.LuaState.Get();
	if (!L)
		return ReturnValue;

	FLuaValue Value = InTable.GetField(Key);
	if (Value.Type == ELuaValueType::Nil)
		return ReturnValue;

	Args.Insert(InTable, 0);

	return LuaValueCall(Value, Args);
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaTableIndexCall(FLuaValue InTable, int32 Index, TArray<FLuaValue> Args)
{
	FLuaValue ReturnValue;
	if (InTable.Type != ELuaValueType::Table)
		return ReturnValue;

	ULuaState* L = InTable.LuaState.Get();
	if (!L)
		return ReturnValue;

	FLuaValue Value = InTable.GetFieldByIndex(Index);
	if (Value.Type == ELuaValueType::Nil)
		return ReturnValue;

	return LuaValueCall(Value, Args);
}

TArray<FLuaValue> ULuaBlueprintFunctionLibrary::LuaTableUnpack(FLuaValue InTable)
{
	TArray<FLuaValue> ReturnValue;
	if (InTable.Type != ELuaValueType::Table)
		return ReturnValue;

	int32 Index = 1;
	for (;;)
	{
		FLuaValue Item = InTable.GetFieldByIndex(Index++);
		if (Item.Type == ELuaValueType::Nil)
			break;
		ReturnValue.Add(Item);
	}

	return ReturnValue;
}

TArray<FLuaValue> ULuaBlueprintFunctionLibrary::LuaTableMergeUnpack(FLuaValue InTable1, FLuaValue InTable2)
{
	TArray<FLuaValue> ReturnValue;
	if (InTable1.Type != ELuaValueType::Table)
		return ReturnValue;

	if (InTable2.Type != ELuaValueType::Table)
		return ReturnValue;

	int32 Index = 1;
	for (;;)
	{
		FLuaValue Item = InTable1.GetFieldByIndex(Index++);
		if (Item.Type == ELuaValueType::Nil)
			break;
		ReturnValue.Add(Item);
	}

	for (;;)
	{
		FLuaValue Item = InTable2.GetFieldByIndex(Index++);
		if (Item.Type == ELuaValueType::Nil)
			break;
		ReturnValue.Add(Item);
	}

	return ReturnValue;
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaTablePack(UObject* WorldContextObject, TSubclassOf<ULuaState> State, TArray<FLuaValue> Values)
{
	FLuaValue ReturnValue;
	ULuaState* L = FLuaMachineModule::Get().GetLuaState(State, WorldContextObject->GetWorld());
	if (!L)
		return ReturnValue;

	ReturnValue = L->CreateLuaTable();

	int32 Index = 1;

	for (FLuaValue& Value : Values)
	{
		ReturnValue.SetFieldByIndex(Index++, Value);
	}

	return ReturnValue;
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaTableMergePack(UObject* WorldContextObject, TSubclassOf<ULuaState> State, TArray<FLuaValue> Values1, TArray<FLuaValue> Values2)
{
	FLuaValue ReturnValue;
	ULuaState* L = FLuaMachineModule::Get().GetLuaState(State, WorldContextObject->GetWorld());
	if (!L)
		return ReturnValue;

	ReturnValue = L->CreateLuaTable();

	int32 Index = 1;

	for (FLuaValue& Value : Values1)
	{
		ReturnValue.SetFieldByIndex(Index++, Value);
	}

	for (FLuaValue& Value : Values2)
	{
		ReturnValue.SetFieldByIndex(Index++, Value);
	}

	return ReturnValue;
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaTableFromMap(UObject* WorldContextObject, TSubclassOf<ULuaState> State, TMap<FString, FLuaValue> Map)
{
	FLuaValue ReturnValue;
	ULuaState* L = FLuaMachineModule::Get().GetLuaState(State, WorldContextObject->GetWorld());
	if (!L)
		return ReturnValue;

	ReturnValue = L->CreateLuaTable();

	for (TPair<FString, FLuaValue>& Pair : Map)
	{
		ReturnValue.SetField(Pair.Key, Pair.Value);
	}

	return ReturnValue;
}

TArray<FLuaValue> ULuaBlueprintFunctionLibrary::LuaTableRange(FLuaValue InTable, int32 First, int32 Last)
{
	TArray<FLuaValue> ReturnValue;
	if (InTable.Type != ELuaValueType::Table)
		return ReturnValue;

	for (int32 i = First; i <= Last; i++)
	{
		ReturnValue.Add(InTable.GetFieldByIndex(i));
	}

	return ReturnValue;
}

TArray<FLuaValue> ULuaBlueprintFunctionLibrary::LuaValueArrayMerge(TArray<FLuaValue> Array1, TArray<FLuaValue> Array2)
{
	TArray<FLuaValue> NewArray = Array1;
	NewArray.Append(Array2);
	return NewArray;
}

TArray<FLuaValue> ULuaBlueprintFunctionLibrary::LuaValueArrayAppend(TArray<FLuaValue> Array, FLuaValue Value)
{
	TArray<FLuaValue> NewArray = Array;
	NewArray.Add(Value);
	return NewArray;
}

TArray<FLuaValue> ULuaBlueprintFunctionLibrary::LuaValueCallMulti(FLuaValue Value, TArray<FLuaValue> Args)
{
	TArray<FLuaValue> ReturnValue;

	ULuaState* L = Value.LuaState.Get();
	if (!L)
		return ReturnValue;

	L->FromLuaValue(Value);

	int32 StackTop = L->GetTop();

	int NArgs = 0;
	for (FLuaValue& Arg : Args)
	{
		L->FromLuaValue(Arg);
		NArgs++;
	}

	FLuaValue LastReturnValue;
	if (L->PCall(NArgs, LastReturnValue, LUA_MULTRET))
	{

		int32 NumOfReturnValues = (L->GetTop() - StackTop) + 1;
		if (NumOfReturnValues > 0)
		{
			for (int32 i = -1; i >= -(NumOfReturnValues); i--)
			{
				ReturnValue.Insert(L->ToLuaValue(i), 0);
			}
			L->Pop(NumOfReturnValues - 1);
		}

	}

	L->Pop();

	return ReturnValue;
}

void ULuaBlueprintFunctionLibrary::LuaValueYield(FLuaValue Value, TArray<FLuaValue> Args)
{
	if (Value.Type != ELuaValueType::Thread)
		return;

	ULuaState* L = Value.LuaState.Get();
	if (!L)
		return;

	L->FromLuaValue(Value);

	int32 StackTop = L->GetTop();

	int NArgs = 0;
	for (FLuaValue& Arg : Args)
	{
		L->FromLuaValue(Arg);
		NArgs++;
	}

	L->Yield(-1 - NArgs, NArgs);

	L->Pop();
}

TArray<FLuaValue> ULuaBlueprintFunctionLibrary::LuaValueResumeMulti(FLuaValue Value, TArray<FLuaValue> Args)
{
	TArray<FLuaValue> ReturnValue;

	if (Value.Type != ELuaValueType::Thread)
		return ReturnValue;

	ULuaState* L = Value.LuaState.Get();
	if (!L)
		return ReturnValue;

	L->FromLuaValue(Value);

	int32 StackTop = L->GetTop();

	int NArgs = 0;
	for (FLuaValue& Arg : Args)
	{
		L->FromLuaValue(Arg);
		NArgs++;
	}

	L->Resume(-1 - NArgs, NArgs);

	int32 NumOfReturnValues = (L->GetTop() - StackTop);
	if (NumOfReturnValues > 0)
	{
		for (int32 i = -1; i >= -(NumOfReturnValues); i--)
		{
			ReturnValue.Insert(L->ToLuaValue(i), 0);
		}
		L->Pop(NumOfReturnValues);
	}

	L->Pop();

	return ReturnValue;
}

FVector ULuaBlueprintFunctionLibrary::LuaTableToVector(FLuaValue Value)
{
	if (Value.Type != ELuaValueType::Table)
	{
		return FVector(NAN);
	}

	auto GetVectorField = [](FLuaValue& Table, const char* Field_n, const char* Field_N, int32 Index) -> FLuaValue
	{
		FLuaValue N = Table.GetField(Field_n);
		if (N.IsNil())
		{
			N = Table.GetField(Field_N);
			if (N.IsNil())
			{
				N = Table.GetFieldByIndex(Index);
				if (N.IsNil())
				{
					N = FLuaValue(NAN);
				}
			}
		}
		return N;
	};

	FLuaValue X = GetVectorField(Value, "x", "X", 1);
	FLuaValue Y = GetVectorField(Value, "y", "Y", 2);
	FLuaValue Z = GetVectorField(Value, "z", "Z", 3);

	return FVector(X.ToFloat(), Y.ToFloat(), Z.ToFloat());
}

FRotator ULuaBlueprintFunctionLibrary::LuaTableToRotator(FLuaValue Value)
{
	if (Value.Type != ELuaValueType::Table)
	{
		return FRotator(NAN);
	}

	auto GetRotatorField = [](FLuaValue& Table, const char* Field_n, const char* Field_N, int32 Index) -> FLuaValue
		{
			FLuaValue N = Table.GetField(Field_n);
			if (N.IsNil())
			{
				N = Table.GetField(Field_N);
				if (N.IsNil())
				{
					N = Table.GetFieldByIndex(Index);
					if (N.IsNil())
					{
						N = FLuaValue(NAN);
					}
				}
			}
			return N;
		};

	FLuaValue Roll = GetRotatorField(Value, "roll", "Roll", 1);
	FLuaValue Pitch = GetRotatorField(Value, "pitch", "Pitch", 2);
	FLuaValue Yaw = GetRotatorField(Value, "yaw", "Yaw", 3);

	return FRotator(Pitch.ToFloat(), Yaw.ToFloat(), Roll.ToFloat());
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaTableSetMetaTable(FLuaValue InTable, FLuaValue InMetaTable)
{
	FLuaValue ReturnValue;
	if (InTable.Type != ELuaValueType::Table || InMetaTable.Type != ELuaValueType::Table)
		return ReturnValue;

	ULuaState* L = InTable.LuaState.Get();
	if (!L)
		return ReturnValue;

	return InTable.SetMetaTable(InMetaTable);
}

int32 ULuaBlueprintFunctionLibrary::LuaValueLength(FLuaValue Value)
{

	ULuaState* L = Value.LuaState.Get();
	if (!L)
		return 0;

	L->FromLuaValue(Value);
	L->Len(-1);
	int32 Length = L->ToInteger(-1);
	L->Pop(2);

	return Length;
}

TArray<FLuaValue> ULuaBlueprintFunctionLibrary::LuaTableGetKeys(FLuaValue Table)
{
	TArray<FLuaValue> Keys;

	if (Table.Type != ELuaValueType::Table)
		return Keys;

	ULuaState* L = Table.LuaState.Get();
	if (!L)
		return Keys;

	L->FromLuaValue(Table);
	L->PushNil(); // first key
	while (L->Next(-2))
	{
		Keys.Add(L->ToLuaValue(-2)); // add key
		L->Pop(); // pop the value
	}

	L->Pop(); // pop the table

	return Keys;
}

TArray<FLuaValue> ULuaBlueprintFunctionLibrary::LuaTableGetValues(FLuaValue Table)
{
	TArray<FLuaValue> Keys;

	if (Table.Type != ELuaValueType::Table)
		return Keys;

	ULuaState* L = Table.LuaState.Get();
	if (!L)
		return Keys;

	L->FromLuaValue(Table);
	L->PushNil(); // first key
	while (L->Next(-2))
	{
		Keys.Add(L->ToLuaValue(-1)); // add value
		L->Pop(); // pop the value
	}

	L->Pop(); // pop the table

	return Keys;
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaTableAssetToLuaTable(UObject* WorldContextObject, TSubclassOf<ULuaState> State, ULuaTableAsset* TableAsset)
{
	ULuaState* L = FLuaMachineModule::Get().GetLuaState(State, WorldContextObject->GetWorld());
	if (!L)
		return FLuaValue();

	return TableAsset->ToLuaTable(L);
}

FLuaValue ULuaBlueprintFunctionLibrary::LuaNewLuaUserDataObject(UObject* WorldContextObject, TSubclassOf<ULuaState> State, TSubclassOf<ULuaUserDataObject> UserDataObjectClass, bool bTrackObject)
{
	ULuaState* L = FLuaMachineModule::Get().GetLuaState(State, WorldContextObject->GetWorld());
	if (!L)
		return FLuaValue();

	return L->NewLuaUserDataObject(UserDataObjectClass, bTrackObject);
}

ULuaState* ULuaBlueprintFunctionLibrary::LuaGetState(UObject* WorldContextObject, TSubclassOf<ULuaState> State)
{
	return FLuaMachineModule::Get().GetLuaState(State, WorldContextObject->GetWorld());
}

bool ULuaBlueprintFunctionLibrary::LuaTableImplements(FLuaValue Table, ULuaTableAsset* TableAsset)
{
	if (Table.Type != ELuaValueType::Table)
		return false;

	ULuaState* L = Table.LuaState.Get();
	if (!L)
		return false;

	for (TPair<FString, FLuaValue>& Pair : TableAsset->Table)
	{
		FLuaValue Item = Table.GetField(Pair.Key);
		if (Item.Type == ELuaValueType::Nil)
			return false;
		if (Item.Type != Pair.Value.Type)
			return false;
	}

	return true;
}

bool ULuaBlueprintFunctionLibrary::LuaTableImplementsAll(FLuaValue Table, TArray<ULuaTableAsset*> TableAssets)
{
	for (ULuaTableAsset* TableAsset : TableAssets)
	{
		if (!LuaTableImplements(Table, TableAsset))
			return false;
	}
	return true;
}

bool ULuaBlueprintFunctionLibrary::LuaTableImplementsAny(FLuaValue Table, TArray<ULuaTableAsset*> TableAssets)
{
	for (ULuaTableAsset* TableAsset : TableAssets)
	{
		if (LuaTableImplements(Table, TableAsset))
			return true;
	}
	return false;
}

int32 ULuaBlueprintFunctionLibrary::LuaGetUsedMemory(UObject* WorldContextObject, TSubclassOf<ULuaState> State)
{
	ULuaState* L = FLuaMachineModule::Get().GetLuaState(State, WorldContextObject->GetWorld());
	if (!L)
		return -1;

	return L->GC(LUA_GCCOUNT);
}

void ULuaBlueprintFunctionLibrary::LuaGCCollect(UObject* WorldContextObject, TSubclassOf<ULuaState> State)
{
	ULuaState* L = FLuaMachineModule::Get().GetLuaState(State, WorldContextObject->GetWorld());
	if (!L)
		return;

	L->GC(LUA_GCCOLLECT);
}

void ULuaBlueprintFunctionLibrary::LuaGCStop(UObject* WorldContextObject, TSubclassOf<ULuaState> State)
{
	ULuaState* L = FLuaMachineModule::Get().GetLuaState(State, WorldContextObject->GetWorld());
	if (!L)
		return;

	L->GC(LUA_GCSTOP);
}

void ULuaBlueprintFunctionLibrary::LuaGCRestart(UObject* WorldContextObject, TSubclassOf<ULuaState> State)
{
	ULuaState* L = FLuaMachineModule::Get().GetLuaState(State, WorldContextObject->GetWorld());
	if (!L)
		return;

	L->GC(LUA_GCRESTART);
}

void ULuaBlueprintFunctionLibrary::LuaSetUserDataMetaTable(UObject* WorldContextObject, TSubclassOf<ULuaState> State, FLuaValue MetaTable)
{
	ULuaState* L = FLuaMachineModule::Get().GetLuaState(State, WorldContextObject->GetWorld());
	if (!L)
		return;
	L->SetUserDataMetaTable(MetaTable);
}

bool ULuaBlueprintFunctionLibrary::LuaValueIsReferencedInLuaRegistry(FLuaValue Value)
{
	return Value.IsReferencedInLuaRegistry();
}

UClass* ULuaBlueprintFunctionLibrary::LuaValueToBlueprintGeneratedClass(const FLuaValue& Value)
{
	UObject* LoadedObject = nullptr;
	if (Value.Type == ELuaValueType::String)
	{
		LoadedObject = StaticLoadObject(UBlueprint::StaticClass(), nullptr, *Value.ToString());
	}
	else if (Value.Type == ELuaValueType::UObject)
	{
		LoadedObject = Value.Object;
	}

	if (!LoadedObject)
		return nullptr;

	UBlueprint* Blueprint = Cast<UBlueprint>(LoadedObject);
	if (!Blueprint)
		return nullptr;
	return Cast<UClass>(Blueprint->GeneratedClass);
}

UClass* ULuaBlueprintFunctionLibrary::LuaValueLoadClass(const FLuaValue& Value, bool bDetectBlueprintGeneratedClass)
{
	UObject* LoadedObject = nullptr;
	if (Value.Type == ELuaValueType::String)
	{
		LoadedObject = StaticLoadObject(UObject::StaticClass(), nullptr, *Value.ToString());
	}
	else if (Value.Type == ELuaValueType::UObject)
	{
		LoadedObject = Value.Object;
	}

	if (!LoadedObject)
		return nullptr;

	if (bDetectBlueprintGeneratedClass)
	{
		UBlueprint* Blueprint = Cast<UBlueprint>(LoadedObject);
		if (Blueprint)
		{
			return Cast<UClass>(Blueprint->GeneratedClass);
		}
		UBlueprintGeneratedClass* BlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>(LoadedObject);
		if (BlueprintGeneratedClass)
		{
			return BlueprintGeneratedClass;
		}
	}

	return Cast<UClass>(LoadedObject);
}

UObject* ULuaBlueprintFunctionLibrary::LuaValueLoadObject(const FLuaValue& Value)
{
	UObject* LoadedObject = nullptr;
	if (Value.Type == ELuaValueType::String)
	{
		LoadedObject = StaticLoadObject(UObject::StaticClass(), nullptr, *Value.ToString());
	}
	else if (Value.Type == ELuaValueType::UObject)
	{
		LoadedObject = Value.Object;
	}

	return LoadedObject;
}

bool ULuaBlueprintFunctionLibrary::LuaValueFromJson(UObject* WorldContextObject, TSubclassOf<ULuaState> State, const FString& Json, FLuaValue& LuaValue)
{
	// default to nil
	LuaValue = FLuaValue();

	ULuaState* L = FLuaMachineModule::Get().GetLuaState(State, WorldContextObject->GetWorld());
	if (!L)
		return false;

	TSharedPtr<FJsonValue> JsonValue;
	TSharedRef< TJsonReader<TCHAR> > JsonReader = TJsonReaderFactory<TCHAR>::Create(Json);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonValue))
	{
		return false;
	}

	LuaValue = FLuaValue::FromJsonValue(L, *JsonValue);
	return true;
}

FString ULuaBlueprintFunctionLibrary::LuaValueToJson(FLuaValue Value)
{
	FString Json;
	TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&Json);
	FJsonSerializer::Serialize(Value.ToJsonValue(), "", JsonWriter);
	return Json;
}

bool ULuaBlueprintFunctionLibrary::LuaLoadPakFile(const FString& Filename, FString Mountpoint, TArray<FLuaValue>& Assets, FString ContentPath, FString AssetRegistryPath)
{
	if (!Mountpoint.StartsWith("/") || !Mountpoint.EndsWith("/"))
	{
		UE_LOG(LogLuaMachine, Error, TEXT("Invalid Mountpoint, must be in the format /Name/"));
		return false;
	}

	IPlatformFile& TopPlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	bool bCustomPakPlatformFile = false;

	FPakPlatformFile* PakPlatformFile = (FPakPlatformFile*)FPlatformFileManager::Get().FindPlatformFile(TEXT("PakFile"));
	if (!PakPlatformFile)
	{
		PakPlatformFile = new FPakPlatformFile();
		if (!PakPlatformFile->Initialize(&TopPlatformFile, TEXT("")))
		{
			UE_LOG(LogLuaMachine, Error, TEXT("Unable to setup PakPlatformFile"));
			delete(PakPlatformFile);
			return false;
		}
		FPlatformFileManager::Get().SetPlatformFile(*PakPlatformFile);
		bCustomPakPlatformFile = true;
	}

#if	ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 26
	TRefCountPtr<FPakFile> PakFile = new FPakFile(PakPlatformFile, *Filename, false);
#else
	FPakFile PakFile(PakPlatformFile, *Filename, false);
#endif
	if (!PakFile.IsValid())
	{
		UE_LOG(LogLuaMachine, Error, TEXT("Unable to open PakFile"));
		if (bCustomPakPlatformFile)
		{
			FPlatformFileManager::Get().SetPlatformFile(TopPlatformFile);
			delete(PakPlatformFile);
		}
		return false;
	}

#if	ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 26
	FString PakFileMountPoint = PakFile->GetMountPoint();
#else
	FString PakFileMountPoint = PakFile.GetMountPoint();
#endif

	FPaths::MakeStandardFilename(Mountpoint);

	FPaths::MakeStandardFilename(PakFileMountPoint);

#if	ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 26
	PakFile->SetMountPoint(*PakFileMountPoint);
#else
	PakFile.SetMountPoint(*PakFileMountPoint);
#endif

	if (!PakPlatformFile->Mount(*Filename, 0, *PakFileMountPoint))
	{
		UE_LOG(LogLuaMachine, Error, TEXT("Unable to mount PakFile"));
		if (bCustomPakPlatformFile)
		{
			FPlatformFileManager::Get().SetPlatformFile(TopPlatformFile);
			delete(PakPlatformFile);
		}
		return false;
	}

	if (ContentPath.IsEmpty())
	{
		ContentPath = "/Plugins" + Mountpoint + "Content/";
	}

	FString MountDestination = PakFileMountPoint + ContentPath;
	FPaths::MakeStandardFilename(MountDestination);

	FPackageName::RegisterMountPoint(Mountpoint, MountDestination);

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

#if WITH_EDITOR
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 23
	int32 bPreviousGAllowUnversionedContentInEditor = GAllowUnversionedContentInEditor;
#else
	bool bPreviousGAllowUnversionedContentInEditor = GAllowUnversionedContentInEditor;
#endif
	GAllowUnversionedContentInEditor = true;
#endif

	if (AssetRegistryPath.IsEmpty())
	{
		AssetRegistryPath = "/Plugins" + Mountpoint + "AssetRegistry.bin";
	}

	FArrayReader SerializedAssetData;
	if (!FFileHelper::LoadFileToArray(SerializedAssetData, *(PakFileMountPoint + AssetRegistryPath)))
	{
		UE_LOG(LogLuaMachine, Error, TEXT("Unable to parse AssetRegistry file"));
		if (bCustomPakPlatformFile)
		{
			FPlatformFileManager::Get().SetPlatformFile(TopPlatformFile);
			delete(PakPlatformFile);
		}
#if WITH_EDITOR
		GAllowUnversionedContentInEditor = bPreviousGAllowUnversionedContentInEditor;
#endif
		return false;
	}

	AssetRegistry.Serialize(SerializedAssetData);

	AssetRegistry.ScanPathsSynchronous({ Mountpoint }, true);

	TArray<FAssetData> AssetData;
	AssetRegistry.GetAllAssets(AssetData, false);

	for (auto Asset : AssetData)
	{
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION > 0
		if (Asset.GetObjectPathString().StartsWith(Mountpoint))
#else
		if (Asset.ObjectPath.ToString().StartsWith(Mountpoint))
#endif
		{
			Assets.Add(FLuaValue(Asset.GetAsset()));
		}
	}

	if (bCustomPakPlatformFile)
	{
		FPlatformFileManager::Get().SetPlatformFile(TopPlatformFile);
		delete(PakPlatformFile);
	}

#if WITH_EDITOR
	GAllowUnversionedContentInEditor = bPreviousGAllowUnversionedContentInEditor;
#endif

	return true;
}

void ULuaBlueprintFunctionLibrary::SwitchOnLuaValueType(const FLuaValue& LuaValue, ELuaValueType& LuaValueTypes)
{
	LuaValueTypes = LuaValue.Type;
}

void ULuaBlueprintFunctionLibrary::GetLuaReflectionType(UObject* InObject, const FString& Name, ELuaReflectionType& LuaReflectionTypes)
{
	LuaReflectionTypes = ELuaReflectionType::Unknown;
	UClass* Class = InObject->GetClass();
	if (!Class)
	{
		return;
	}

	if (Class->FindPropertyByName(FName(*Name)) != nullptr)
	{
		LuaReflectionTypes = ELuaReflectionType::Property;
		return;
	}

	if (Class->FindFunctionByName(FName(*Name)))
	{
		LuaReflectionTypes = ELuaReflectionType::Function;
		return;
	}
}

void ULuaBlueprintFunctionLibrary::RegisterLuaConsoleCommand(const FString& CommandName, const FLuaValue& LuaConsoleCommand)
{
	FLuaMachineModule::Get().RegisterLuaConsoleCommand(CommandName, LuaConsoleCommand);
}

void ULuaBlueprintFunctionLibrary::UnregisterLuaConsoleCommand(const FString& CommandName)
{
	FLuaMachineModule::Get().UnregisterLuaConsoleCommand(CommandName);
}

ULuaState* ULuaBlueprintFunctionLibrary::CreateDynamicLuaState(UObject* WorldContextObject, TSubclassOf<ULuaState> LuaStateClass)
{
	if (!LuaStateClass)
	{
		return nullptr;
	}

	if (LuaStateClass == ULuaState::StaticClass())
	{
		UE_LOG(LogLuaMachine, Error, TEXT("attempt to use LuaState Abstract class, please create a child of LuaState"));
		return nullptr;
	}


	ULuaState* NewLuaState = NewObject<ULuaState>((UObject*)GetTransientPackage(), LuaStateClass);
	if (!NewLuaState)
	{
		return nullptr;
	}

	return NewLuaState->GetLuaState(WorldContextObject->GetWorld());
}
