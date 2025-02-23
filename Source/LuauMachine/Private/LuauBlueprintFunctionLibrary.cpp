// Copyright 2018-2023 - Roberto De Ioris

#include "LuauBlueprintFunctionLibrary.h"
#include "LuauComponent.h"
#include "LuauMachine.h"
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

FLuauValue ULuauBlueprintFunctionLibrary::LuauCreateNil()
{
	return FLuauValue();
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauCreateString(const FString& String)
{
	return FLuauValue(String);
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauCreateNumber(const float Value)
{
	return FLuauValue(Value);
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauCreateInteger(const int32 Value)
{
	return FLuauValue(Value);
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauCreateInteger64(const int64 Value)
{
	return FLuauValue(Value);
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauCreateBool(const bool bInBool)
{
	return FLuauValue(bInBool);
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauCreateObject(UObject* InObject)
{
	return FLuauValue(InObject);
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauCreateUFunction(UObject* InObject, const FString& FunctionName)
{
	if (InObject && InObject->FindFunction(FName(*FunctionName)))
	{
		FLuauValue Value = FLuauValue::Function(FName(*FunctionName));
		Value.Object = InObject;
		return Value;
	}

	return FLuauValue();
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauCreateTable(UObject* WorldContextObject, TSubclassOf<ULuauState> State)
{
	FLuauValue LuauValue;
	ULuauState* L = FLuauMachineModule::Get().GetLuauState(State, WorldContextObject->GetWorld());
	if (!L)
		return LuauValue;

	return L->CreateLuauTable();
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauCreateLazyTable(UObject* WorldContextObject, TSubclassOf<ULuauState> State)
{
	FLuauValue LuauValue;
	ULuauState* L = FLuauMachineModule::Get().GetLuauState(State, WorldContextObject->GetWorld());
	if (!L)
		return LuauValue;

	return L->CreateLuauLazyTable();
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauCreateThread(UObject* WorldContextObject, TSubclassOf<ULuauState> State, FLuauValue Value)
{
	FLuauValue LuauValue;
	ULuauState* L = FLuauMachineModule::Get().GetLuauState(State, WorldContextObject->GetWorld());
	if (!L)
		return LuauValue;

	return L->CreateLuauThread(Value);
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauCreateObjectInState(UObject* WorldContextObject, TSubclassOf<ULuauState> State, UObject* InObject)
{
	FLuauValue LuauValue;
	if (!InObject)
		return LuauValue;
	ULuauState* L = FLuauMachineModule::Get().GetLuauState(State, WorldContextObject->GetWorld());
	if (!L)
		return LuauValue;

	LuauValue = FLuauValue(InObject);
	LuauValue.LuauState = L;
	return LuauValue;
}

void ULuauBlueprintFunctionLibrary::LuauStateDestroy(UObject* WorldContextObject, TSubclassOf<ULuauState> State)
{
	ULuauState* L = FLuauMachineModule::Get().GetLuauState(State, WorldContextObject->GetWorld());
	if (!L)
		return;
	FLuauMachineModule::Get().UnregisterLuauState(L);
}

void ULuauBlueprintFunctionLibrary::LuauStateReload(UObject* WorldContextObject, TSubclassOf<ULuauState> State)
{
	ULuauState* L = FLuauMachineModule::Get().GetLuauState(State, WorldContextObject->GetWorld());
	if (!L)
		return;
	FLuauMachineModule::Get().UnregisterLuauState(L);
	FLuauMachineModule::Get().GetLuauState(State, WorldContextObject->GetWorld());
}

FString ULuauBlueprintFunctionLibrary::Conv_LuauValueToString(const FLuauValue& Value)
{
	return Value.ToString();
}

FVector ULuauBlueprintFunctionLibrary::Conv_LuauValueToFVector(const FLuauValue& Value)
{
	return LuauTableToVector(Value);
}

FRotator ULuauBlueprintFunctionLibrary::Conv_LuauValueToFRotator(const FLuauValue& Value)
{
	return LuauTableToRotator(Value);
}

FName ULuauBlueprintFunctionLibrary::Conv_LuauValueToName(const FLuauValue& Value)
{
	return FName(*Value.ToString());
}

FText ULuauBlueprintFunctionLibrary::Conv_LuauValueToText(const FLuauValue& Value)
{
	return FText::FromString(Value.ToString());
}

UObject* ULuauBlueprintFunctionLibrary::Conv_LuauValueToObject(const FLuauValue& Value)
{
	if (Value.Type == ELuauValueType::UObject)
	{
		return Value.Object;
	}
	return nullptr;
}

UClass* ULuauBlueprintFunctionLibrary::Conv_LuauValueToClass(const FLuauValue& Value)
{
	if (Value.Type == ELuauValueType::UObject)
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

FLuauValue ULuauBlueprintFunctionLibrary::Conv_ObjectToLuauValue(UObject* Object)
{
	return FLuauValue(Object);
}


FLuauValue ULuauBlueprintFunctionLibrary::Conv_FloatToLuauValue(const float Value)
{
	return FLuauValue(Value);
}

FLuauValue ULuauBlueprintFunctionLibrary::Conv_BoolToLuauValue(const bool Value)
{
	return FLuauValue(Value);
}

int32 ULuauBlueprintFunctionLibrary::Conv_LuauValueToInt(const FLuauValue& Value)
{
	return Value.ToInteger();
}

int64 ULuauBlueprintFunctionLibrary::Conv_LuauValueToInt64(const FLuauValue& Value)
{
	return Value.ToInteger();
}

float ULuauBlueprintFunctionLibrary::Conv_LuauValueToFloat(const FLuauValue& Value)
{
	return Value.ToFloat();
}

bool ULuauBlueprintFunctionLibrary::Conv_LuauValueToBool(const FLuauValue& Value)
{
	return Value.ToBool();
}

FLuauValue ULuauBlueprintFunctionLibrary::Conv_IntToLuauValue(const int32 Value)
{
	return FLuauValue(Value);
}

FLuauValue ULuauBlueprintFunctionLibrary::Conv_Int64ToLuauValue(const int64 Value)
{
	return FLuauValue(Value);
}

FLuauValue ULuauBlueprintFunctionLibrary::Conv_StringToLuauValue(const FString& Value)
{
	return FLuauValue(Value);
}

FLuauValue ULuauBlueprintFunctionLibrary::Conv_TextToLuauValue(const FText& Value)
{
	return FLuauValue(Value.ToString());
}

FLuauValue ULuauBlueprintFunctionLibrary::Conv_NameToLuauValue(const FName Value)
{
	return FLuauValue(Value.ToString());
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauGetGlobal(UObject* WorldContextObject, TSubclassOf<ULuauState> State, const FString& Name)
{
	ULuauState* L = FLuauMachineModule::Get().GetLuauState(State, WorldContextObject->GetWorld());
	if (!L)
		return FLuauValue();

	uint32 ItemsToPop = L->GetFieldFromTree(Name);
	FLuauValue ReturnValue = L->ToLuauValue(-1);
	L->Pop(ItemsToPop);
	return ReturnValue;
}

int64 ULuauBlueprintFunctionLibrary::LuauValueToPointer(UObject* WorldContextObject, TSubclassOf<ULuauState> State, FLuauValue Value)
{
	ULuauState* L = FLuauMachineModule::Get().GetLuauState(State, WorldContextObject->GetWorld());
	if (!L)
		return 0;

	L->FromLuauValue(Value);
	const void* Ptr = L->ToPointer(-1);
	L->Pop();

	return (int64)Ptr;
}

FString ULuauBlueprintFunctionLibrary::LuauValueToHexPointer(UObject* WorldContextObject, TSubclassOf<ULuauState> State, FLuauValue Value)
{
	int64 Ptr = LuauValueToPointer(WorldContextObject, State, Value);
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

FString ULuauBlueprintFunctionLibrary::LuauValueToBase64(const FLuauValue& Value)
{
	return Value.ToBase64();
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauValueFromBase64(const FString& Base64)
{
	return FLuauValue::FromBase64(Base64);
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauValueFromUTF16(const FString& String)
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
	return FLuauValue(Bytes);
}

FString ULuauBlueprintFunctionLibrary::LuauValueToUTF16(const FLuauValue& Value)
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

FLuauValue ULuauBlueprintFunctionLibrary::LuauValueFromUTF8(const FString& String)
{
	FTCHARToUTF8 UTF8String(*String);
	return FLuauValue((const char*)UTF8String.Get(), UTF8String.Length());
}

FString ULuauBlueprintFunctionLibrary::LuauValueToUTF8(const FLuauValue& Value)
{
	FString ReturnValue;
	TArray<uint8> Bytes = Value.ToBytes();
	Bytes.Add(0);
	return FString(UTF8_TO_TCHAR(Bytes.GetData()));
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauValueFromUTF32(const FString& String)
{
#if ENGINE_MINOR_VERSION >= 25
	FTCHARToUTF32 UTF32String(*String);
	return FLuauValue((const char*)UTF32String.Get(), UTF32String.Length());
#else
	UE_LOG(LogLuauMachine, Error, TEXT("UTF32 not supported in this engine version"));
	return FLuauValue();
#endif
}

FString ULuauBlueprintFunctionLibrary::LuauValueToUTF32(const FLuauValue& Value)
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
	UE_LOG(LogLuauMachine, Error, TEXT("UTF32 not supported in this engine version"));
	return FString("");
#endif
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauRunFile(UObject* WorldContextObject, TSubclassOf<ULuauState> State, const FString& Filename, const bool bIgnoreNonExistent)
{
	FLuauValue ReturnValue;

	ULuauState* L = FLuauMachineModule::Get().GetLuauState(State, WorldContextObject->GetWorld());
	if (!L)
		return ReturnValue;

	if (!L->RunFile(Filename, bIgnoreNonExistent, 1))
	{
		if (L->bLogError)
			L->LogError(L->LastError);
		L->ReceiveLuauError(L->LastError);
	}
	else
	{
		ReturnValue = L->ToLuauValue(-1);
	}

	L->Pop();
	return ReturnValue;
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauRunNonContentFile(UObject* WorldContextObject, TSubclassOf<ULuauState> State, const FString& Filename, const bool bIgnoreNonExistent)
{
	FLuauValue ReturnValue;

	ULuauState* L = FLuauMachineModule::Get().GetLuauState(State, WorldContextObject->GetWorld());
	if (!L)
		return ReturnValue;

	if (!L->RunFile(Filename, bIgnoreNonExistent, 1, true))
	{
		if (L->bLogError)
			L->LogError(L->LastError);
		L->ReceiveLuauError(L->LastError);
	}
	else
	{
		ReturnValue = L->ToLuauValue(-1);
	}

	L->Pop();
	return ReturnValue;
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauRunString(UObject* WorldContextObject, TSubclassOf<ULuauState> State, const FString& CodeString, FString CodePath)
{
	FLuauValue ReturnValue;

	ULuauState* L = FLuauMachineModule::Get().GetLuauState(State, WorldContextObject->GetWorld());
	if (!L)
	{
		return ReturnValue;
	}

	return L->RunString(CodeString, CodePath);
}

ELuauThreadStatus ULuauBlueprintFunctionLibrary::LuauThreadGetStatus(FLuauValue Value)
{
	if (Value.Type != ELuauValueType::Thread || !Value.LuauState.IsValid())
		return ELuauThreadStatus::Invalid;

	return Value.LuauState->GetLuauThreadStatus(Value);
}

int32 ULuauBlueprintFunctionLibrary::LuauThreadGetStackTop(FLuauValue Value)
{
	if (Value.Type != ELuauValueType::Thread || !Value.LuauState.IsValid())
	{
		return MIN_int32;
	}

	return Value.LuauState->GetLuauThreadStackTop(Value);
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauRunCodeAsset(UObject* WorldContextObject, TSubclassOf<ULuauState> State, ULuauCode* CodeAsset)
{
	FLuauValue ReturnValue;

	if (!CodeAsset)
		return ReturnValue;

	ULuauState* L = FLuauMachineModule::Get().GetLuauState(State, WorldContextObject->GetWorld());
	if (!L)
		return ReturnValue;

	if (!L->RunCodeAsset(CodeAsset, 1))
	{
		if (L->bLogError)
			L->LogError(L->LastError);
		L->ReceiveLuauError(L->LastError);
	}
	else {
		ReturnValue = L->ToLuauValue(-1);
	}
	L->Pop();
	return ReturnValue;
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauRunByteCode(UObject* WorldContextObject, TSubclassOf<ULuauState> State, const TArray<uint8>& ByteCode, const FString& CodePath)
{
	FLuauValue ReturnValue;

	ULuauState* L = FLuauMachineModule::Get().GetLuauState(State, WorldContextObject->GetWorld());
	if (!L)
		return ReturnValue;

	if (!L->RunCode(ByteCode, CodePath, 1))
	{
		if (L->bLogError)
			L->LogError(L->LastError);
		L->ReceiveLuauError(L->LastError);
	}
	else
	{
		ReturnValue = L->ToLuauValue(-1);
	}
	L->Pop();
	return ReturnValue;
}

UTexture2D* ULuauBlueprintFunctionLibrary::LuauValueToTransientTexture(int32 Width, int32 Height, const FLuauValue& Value, EPixelFormat PixelFormat, bool bDetectFormat)
{
	if (Value.Type != ELuauValueType::String)
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
			UE_LOG(LogLuauMachine, Error, TEXT("Unable to detect image format"));
			return nullptr;
		}

		TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(ImageFormat);
		if (!ImageWrapper.IsValid())
		{
			UE_LOG(LogLuauMachine, Error, TEXT("Unable to create ImageWrapper"));
			return nullptr;
		}

		if (!ImageWrapper->SetCompressed(Bytes.GetData(), Bytes.Num()))
		{
			UE_LOG(LogLuauMachine, Error, TEXT("Unable to parse image data"));
			return nullptr;
		}

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 25
		TArray<uint8> UncompressedBytes;
#else
		const TArray<uint8>* UncompressedBytes = nullptr;
#endif
		if (!ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedBytes))
		{
			UE_LOG(LogLuauMachine, Error, TEXT("Unable to get raw image data"));
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

void ULuauBlueprintFunctionLibrary::LuauHttpRequest(UObject* WorldContextObject, TSubclassOf<ULuauState> State, const FString& Method, const FString& URL, TMap<FString, FString> Headers, FLuauValue Body, FLuauValue Context, const FLuauHttpResponseReceived& ResponseReceived, const FLuauHttpError& Error)
{
	ULuauState* L = FLuauMachineModule::Get().GetLuauState(State, WorldContextObject->GetWorld());
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

	TSharedRef<FLuauSmartReference> ContextSmartRef = L->AddLuauSmartReference(Context);

	HttpRequest->OnProcessRequestComplete().BindStatic(&ULuauBlueprintFunctionLibrary::HttpGenericRequestDone, TWeakPtr<FLuauSmartReference>(ContextSmartRef), ResponseReceived, Error);
	HttpRequest->ProcessRequest();
}

void ULuauBlueprintFunctionLibrary::HttpGenericRequestDone(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, TWeakPtr<FLuauSmartReference> Context, FLuauHttpResponseReceived ResponseReceived, FLuauHttpError Error)
{
	// if the context is invalid, the LuauState is already dead
	if (!Context.IsValid())
		return;

	TSharedRef<FLuauSmartReference> SmartContext = Context.Pin().ToSharedRef();

	SmartContext->LuauState->RemoveLuauSmartReference(SmartContext);

	if (!bWasSuccessful)
	{
		Error.ExecuteIfBound(SmartContext->Value);
		return;
	}

	FLuauValue StatusCode = FLuauValue(Response->GetResponseCode());
	FLuauValue Headers = SmartContext->LuauState->CreateLuauTable();
	for (auto HeaderLine : Response->GetAllHeaders())
	{
		int32 Index;
		if (HeaderLine.Len() > 2 && HeaderLine.FindChar(':', Index))
		{
			FString Key;
			if (Index > 0)
				Key = HeaderLine.Left(Index);
			FString Value = HeaderLine.Right(HeaderLine.Len() - (Index + 2));
			Headers.SetField(Key, FLuauValue(Value));
		}
	}
	FLuauValue Content = FLuauValue(Response->GetContent());
	FLuauValue LuauHttpResponse = SmartContext->LuauState->CreateLuauTable();
	LuauHttpResponse.SetFieldByIndex(1, StatusCode);
	LuauHttpResponse.SetFieldByIndex(2, Headers);
	LuauHttpResponse.SetFieldByIndex(3, Content);
	ResponseReceived.ExecuteIfBound(SmartContext->Value, LuauHttpResponse);
}

void ULuauBlueprintFunctionLibrary::LuauRunURL(UObject* WorldContextObject, TSubclassOf<ULuauState> State, const FString& URL, TMap<FString, FString> Headers, const FString& SecurityHeader, const FString& SignaturePublicExponent, const FString& SignatureModulus, FLuauHttpSuccess Completed)
{

	// Security CHECK
	if (!SecurityHeader.IsEmpty() || !SignaturePublicExponent.IsEmpty() || !SignatureModulus.IsEmpty())
	{
		if (SecurityHeader.IsEmpty() || SignaturePublicExponent.IsEmpty() || SignatureModulus.IsEmpty())
		{
			UE_LOG(LogLuauMachine, Error, TEXT("For secure LuauRunURL() you need to specify the Security HTTP Header, the Signature Public Exponent and the Signature Modulus"));
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
	HttpRequest->OnProcessRequestComplete().BindStatic(&ULuauBlueprintFunctionLibrary::HttpRequestDone, State, TWeakObjectPtr<UWorld>(WorldContextObject->GetWorld()), SecurityHeader, SignaturePublicExponent, SignatureModulus, Completed);
	HttpRequest->ProcessRequest();
}

void ULuauBlueprintFunctionLibrary::HttpRequestDone(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, TSubclassOf<ULuauState> LuauState, TWeakObjectPtr<UWorld> World, const FString SecurityHeader, const FString SignaturePublicExponent, const FString SignatureModulus, FLuauHttpSuccess Completed)
{
	FLuauValue ReturnValue;
	int32 StatusCode = -1;

	if (!bWasSuccessful)
	{
		UE_LOG(LogLuauMachine, Error, TEXT("HTTP session failed for \"%s %s\""), *Request->GetVerb(), *Request->GetURL());
	}

	else if (!World.IsValid())
	{
		UE_LOG(LogLuauMachine, Error, TEXT("Unable to access LuauState as the World object is no more available (\"%s %s\")"), *Request->GetVerb(), *Request->GetURL());
	}
	else
	{
		// Check signature
		if (!SecurityHeader.IsEmpty())
		{
			// check code size
			if (Response->GetContentLength() <= 0)
			{
				UE_LOG(LogLuauMachine, Error, TEXT("[Security] Invalid Content Size"));
				Completed.ExecuteIfBound(ReturnValue, bWasSuccessful, StatusCode);
				return;
			}
			FString EncryptesSignatureBase64 = Response->GetHeader(SecurityHeader);
			if (EncryptesSignatureBase64.IsEmpty())
			{
				UE_LOG(LogLuauMachine, Error, TEXT("[Security] Invalid Security HTTP Header"));
				Completed.ExecuteIfBound(ReturnValue, bWasSuccessful, StatusCode);
				return;
			}

			const uint32 KeySize = 64;

			TArray<uint8> Signature;
			FBase64::Decode(EncryptesSignatureBase64, Signature);

			if (Signature.Num() != KeySize)
			{
				UE_LOG(LogLuauMachine, Error, TEXT("[Security] Invalid Signature"));
				Completed.ExecuteIfBound(ReturnValue, bWasSuccessful, StatusCode);
				return;
			}
			TEncryptionInt SignatureValue = TEncryptionInt((uint32*)&Signature[0]);

			TArray<uint8> PublicExponent;
			FBase64::Decode(SignaturePublicExponent, PublicExponent);
			if (PublicExponent.Num() != KeySize)
			{
				UE_LOG(LogLuauMachine, Error, TEXT("[Security] Invalid Signature Public Exponent"));
				Completed.ExecuteIfBound(ReturnValue, bWasSuccessful, StatusCode);
				return;
			}

			TArray<uint8> Modulus;
			FBase64::Decode(SignatureModulus, Modulus);
			if (Modulus.Num() != KeySize)
			{
				UE_LOG(LogLuauMachine, Error, TEXT("[Security] Invalid Signature Modulus"));
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
				UE_LOG(LogLuauMachine, Error, TEXT("[Security] Signature check failed"));
				Completed.ExecuteIfBound(ReturnValue, bWasSuccessful, StatusCode);
				return;
			}
		}

		ReturnValue = LuauRunString(World.Get(), LuauState, Response->GetContentAsString());
		StatusCode = Response->GetResponseCode();
	}

	Completed.ExecuteIfBound(ReturnValue, bWasSuccessful, StatusCode);
}

void ULuauBlueprintFunctionLibrary::LuauTableFillObject(FLuauValue InTable, UObject* InObject)
{
	if (InTable.Type != ELuauValueType::Table || !InObject)
		return;

	ULuauState* L = InTable.LuauState.Get();
	if (!L)
		return;

	UStruct* Class = Cast<UStruct>(InObject);
	if (!Class)
		Class = InObject->GetClass();

	L->FromLuauValue(InTable);
	L->PushNil(); // first key
	while (L->Next(-2))
	{
		FLuauValue Key = L->ToLuauValue(-2);
		FLuauValue Value = L->ToLuauValue(-1);
		L->SetPropertyFromLuauValue(InObject, Key.ToString(), Value);
		L->Pop(); // pop the value
	}

	L->Pop(); // pop the table
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauTableGetField(FLuauValue Table, const FString& Key)
{
	ULuauState* L = Table.LuauState.Get();
	if (!L)
		return FLuauValue();

	return Table.GetField(Key);
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauComponentGetField(FLuauValue LuauComponent, const FString& Key)
{
	FLuauValue ReturnValue;
	if (LuauComponent.Type != ELuauValueType::UObject)
		return ReturnValue;

	ULuauState* L = LuauComponent.LuauState.Get();
	if (!L)
		return ReturnValue;

	ULuauComponent* Component = Cast<ULuauComponent>(LuauComponent.Object);

	FLuauValue* LuauValue = Component->Table.Find(Key);
	if (LuauValue)
	{
		return *LuauValue;
	}

	return ReturnValue;
}

bool ULuauBlueprintFunctionLibrary::LuauValueIsNil(const FLuauValue& Value)
{
	return Value.Type == ELuauValueType::Nil;
}

bool ULuauBlueprintFunctionLibrary::LuauValueIsOwned(const FLuauValue& Value)
{
	return Value.LuauState != nullptr;
}

TSubclassOf<ULuauState> ULuauBlueprintFunctionLibrary::LuauValueGetOwner(const FLuauValue& Value)
{
	if (!Value.LuauState.IsValid())
	{
		return nullptr;
	}
	return Value.LuauState->GetClass();
}

bool ULuauBlueprintFunctionLibrary::LuauValueIsNotNil(const FLuauValue& Value)
{
	return Value.Type != ELuauValueType::Nil;
}

bool ULuauBlueprintFunctionLibrary::LuauValueIsTable(const FLuauValue& Value)
{
	return Value.Type == ELuauValueType::Table;
}

bool ULuauBlueprintFunctionLibrary::LuauValueIsBoolean(const FLuauValue& Value)
{
	return Value.Type == ELuauValueType::Bool;
}

bool ULuauBlueprintFunctionLibrary::LuauValueIsThread(const FLuauValue& Value)
{
	return Value.Type == ELuauValueType::Thread;
}

bool ULuauBlueprintFunctionLibrary::LuauValueIsFunction(const FLuauValue& Value)
{
	return Value.Type == ELuauValueType::Function;
}

bool ULuauBlueprintFunctionLibrary::LuauValueIsNumber(const FLuauValue& Value)
{
	return Value.Type == ELuauValueType::Number;
}

bool ULuauBlueprintFunctionLibrary::LuauValueIsInteger(const FLuauValue& Value)
{
	return Value.Type == ELuauValueType::Integer;
}

bool ULuauBlueprintFunctionLibrary::LuauValueIsString(const FLuauValue& Value)
{
	return Value.Type == ELuauValueType::String;
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauTableGetByIndex(FLuauValue Table, int32 Index)
{
	if (Table.Type != ELuauValueType::Table)
		return FLuauValue();

	ULuauState* L = Table.LuauState.Get();
	if (!L)
		return FLuauValue();

	return Table.GetFieldByIndex(Index);
}

FLuauValue ULuauBlueprintFunctionLibrary::AssignLuauValueToLuauState(UObject* WorldContextObject, FLuauValue Value, TSubclassOf<ULuauState> State)
{
	ULuauState* L = FLuauMachineModule::Get().GetLuauState(State, WorldContextObject->GetWorld());
	if (!L)
		return Value;
	Value.LuauState = L;
	return Value;
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauTableSetByIndex(FLuauValue Table, int32 Index, FLuauValue Value)
{
	if (Table.Type != ELuauValueType::Table)
		return FLuauValue();

	ULuauState* L = Table.LuauState.Get();
	if (!L)
		return FLuauValue();

	return Table.SetFieldByIndex(Index, Value);
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauTableSetField(FLuauValue Table, const FString& Key, FLuauValue Value)
{
	FLuauValue ReturnValue;
	if (Table.Type != ELuauValueType::Table)
		return ReturnValue;

	ULuauState* L = Table.LuauState.Get();
	if (!L)
		return ReturnValue;

	return Table.SetField(Key, Value);
}

FLuauValue ULuauBlueprintFunctionLibrary::GetLuauComponentAsLuauValue(AActor* Actor)
{
	if (!Actor)
		return FLuauValue();

	return FLuauValue(Actor->GetComponentByClass(ULuauComponent::StaticClass()));
}

FLuauValue ULuauBlueprintFunctionLibrary::GetLuauComponentByStateAsLuauValue(AActor* Actor, TSubclassOf<ULuauState> State)
{
	if (!Actor)
		return FLuauValue();
#if ENGINE_MAJOR_VERSION < 5 && ENGINE_MINOR_VERSION < 24
	TArray<UActorComponent*> Components = Actor->GetComponentsByClass(ULuauComponent::StaticClass());
#else
	TArray<UActorComponent*> Components;
	Actor->GetComponents(Components);
#endif
	for (UActorComponent* Component : Components)
	{
		ULuauComponent* LuauComponent = Cast<ULuauComponent>(Component);
		if (LuauComponent)
		{
			if (LuauComponent->LuauState == State)
			{
				return FLuauValue(LuauComponent);
			}
		}
	}

	return FLuauValue();
}

FLuauValue ULuauBlueprintFunctionLibrary::GetLuauComponentByNameAsLuauValue(AActor* Actor, const FString& Name)
{
	if (!Actor)
		return FLuauValue();

#if ENGINE_MAJOR_VERSION < 5 && ENGINE_MINOR_VERSION < 24
	TArray<UActorComponent*> Components = Actor->GetComponentsByClass(ULuauComponent::StaticClass());
#else
	TArray<UActorComponent*> Components;
	Actor->GetComponents(Components);
#endif
	for (UActorComponent* Component : Components)
	{
		ULuauComponent* LuauComponent = Cast<ULuauComponent>(Component);
		if (LuauComponent)
		{
			if (LuauComponent->GetName() == Name)
			{
				return FLuauValue(LuauComponent);
			}
		}
	}

	return FLuauValue();
}

FLuauValue ULuauBlueprintFunctionLibrary::GetLuauComponentByStateAndNameAsLuauValue(AActor* Actor, TSubclassOf<ULuauState> State, const FString& Name)
{
	if (!Actor)
		return FLuauValue();

#if ENGINE_MAJOR_VERSION < 5 && ENGINE_MINOR_VERSION < 24
	TArray<UActorComponent*> Components = Actor->GetComponentsByClass(ULuauComponent::StaticClass());
#else
	TArray<UActorComponent*> Components;
	Actor->GetComponents(Components);
#endif
	for (UActorComponent* Component : Components)
	{
		ULuauComponent* LuauComponent = Cast<ULuauComponent>(Component);
		if (LuauComponent)
		{
			if (LuauComponent->LuauState == State && LuauComponent->GetName() == Name)
			{
				return FLuauValue(LuauComponent);
			}
		}
	}

	return FLuauValue();
}

int32 ULuauBlueprintFunctionLibrary::LuauGetTop(UObject* WorldContextObject, TSubclassOf<ULuauState> State)
{
	ULuauState* L = FLuauMachineModule::Get().GetLuauState(State, WorldContextObject->GetWorld());
	if (!L)
		return MIN_int32;
	return L->GetTop();
}

void ULuauBlueprintFunctionLibrary::LuauSetGlobal(UObject* WorldContextObject, TSubclassOf<ULuauState> State, const FString& Name, FLuauValue Value)
{
	ULuauState* L = FLuauMachineModule::Get().GetLuauState(State, WorldContextObject->GetWorld());
	if (!L)
		return;
	L->SetFieldFromTree(Name, Value, true);
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauGlobalCall(UObject* WorldContextObject, TSubclassOf<ULuauState> State, const FString& Name, TArray<FLuauValue> Args)
{
	FLuauValue ReturnValue;
	ULuauState* L = FLuauMachineModule::Get().GetLuauState(State, WorldContextObject->GetWorld());
	if (!L)
		return ReturnValue;

	int32 ItemsToPop = L->GetFieldFromTree(Name);

	int NArgs = 0;
	for (FLuauValue& Arg : Args)
	{
		L->FromLuauValue(Arg);
		NArgs++;
	}

	L->PCall(NArgs, ReturnValue);

	// we have the return value and the function has been removed, so we do not need to change ItemsToPop
	L->Pop(ItemsToPop);

	return ReturnValue;
}

TArray<FLuauValue> ULuauBlueprintFunctionLibrary::LuauGlobalCallMulti(UObject* WorldContextObject, TSubclassOf<ULuauState> State, const FString& Name, TArray<FLuauValue> Args)
{
	TArray<FLuauValue> ReturnValue;
	ULuauState* L = FLuauMachineModule::Get().GetLuauState(State, WorldContextObject->GetWorld());
	if (!L)
		return ReturnValue;

	int32 ItemsToPop = L->GetFieldFromTree(Name);

	int32 StackTop = L->GetTop();

	int NArgs = 0;
	for (FLuauValue& Arg : Args)
	{
		L->FromLuauValue(Arg);
		NArgs++;
	}

	FLuauValue LastReturnValue;
	if (L->PCall(NArgs, LastReturnValue, LUA_MULTRET))
	{

		int32 NumOfReturnValues = (L->GetTop() - StackTop) + 1;
		if (NumOfReturnValues > 0)
		{
			for (int32 i = -1; i >= -(NumOfReturnValues); i--)
			{
				ReturnValue.Insert(L->ToLuauValue(i), 0);
			}
			L->Pop(NumOfReturnValues - 1);
		}

	}

	// we have the return value and the function has been removed, so we do not need to change ItemsToPop
	L->Pop(ItemsToPop);

	return ReturnValue;
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauGlobalCallValue(UObject* WorldContextObject, TSubclassOf<ULuauState> State, FLuauValue Value, TArray<FLuauValue> Args)
{
	FLuauValue ReturnValue;
	ULuauState* L = FLuauMachineModule::Get().GetLuauState(State, WorldContextObject->GetWorld());
	if (!L)
		return ReturnValue;

	L->FromLuauValue(Value);

	int NArgs = 0;
	for (FLuauValue& Arg : Args)
	{
		L->FromLuauValue(Arg);
		NArgs++;
	}

	L->PCall(NArgs, ReturnValue);

	L->Pop();

	return ReturnValue;
}

TArray<FLuauValue> ULuauBlueprintFunctionLibrary::LuauGlobalCallValueMulti(UObject* WorldContextObject, TSubclassOf<ULuauState> State, FLuauValue Value, TArray<FLuauValue> Args)
{
	TArray<FLuauValue> ReturnValue;
	ULuauState* L = FLuauMachineModule::Get().GetLuauState(State, WorldContextObject->GetWorld());
	if (!L)
		return ReturnValue;

	L->FromLuauValue(Value);

	int32 StackTop = L->GetTop();

	int NArgs = 0;
	for (FLuauValue& Arg : Args)
	{
		L->FromLuauValue(Arg);
		NArgs++;
	}

	FLuauValue LastReturnValue;
	if (L->PCall(NArgs, LastReturnValue, LUA_MULTRET))
	{

		int32 NumOfReturnValues = (L->GetTop() - StackTop) + 1;
		if (NumOfReturnValues > 0)
		{
			for (int32 i = -1; i >= -(NumOfReturnValues); i--)
			{
				ReturnValue.Insert(L->ToLuauValue(i), 0);
			}
			L->Pop(NumOfReturnValues - 1);
		}

	}

	L->Pop();

	return ReturnValue;
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauValueCall(FLuauValue Value, TArray<FLuauValue> Args)
{
	FLuauValue ReturnValue;

	ULuauState* L = Value.LuauState.Get();
	if (!L)
		return ReturnValue;

	L->FromLuauValue(Value);

	int NArgs = 0;
	for (FLuauValue& Arg : Args)
	{
		L->FromLuauValue(Arg);
		NArgs++;
	}

	L->PCall(NArgs, ReturnValue);

	L->Pop();

	return ReturnValue;
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauValueCallIfNotNil(FLuauValue Value, TArray<FLuauValue> Args)
{
	FLuauValue ReturnValue;
	if (Value.Type != ELuauValueType::Nil)
		ReturnValue = LuauValueCall(Value, Args);

	return ReturnValue;
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauTableKeyCall(FLuauValue InTable, const FString& Key, TArray<FLuauValue> Args)
{
	FLuauValue ReturnValue;
	if (InTable.Type != ELuauValueType::Table)
		return ReturnValue;

	ULuauState* L = InTable.LuauState.Get();
	if (!L)
		return ReturnValue;

	FLuauValue Value = InTable.GetField(Key);
	if (Value.Type == ELuauValueType::Nil)
		return ReturnValue;

	return LuauValueCall(Value, Args);
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauTableKeyCallWithSelf(FLuauValue InTable, const FString& Key, TArray<FLuauValue> Args)
{
	FLuauValue ReturnValue;
	if (InTable.Type != ELuauValueType::Table)
		return ReturnValue;

	ULuauState* L = InTable.LuauState.Get();
	if (!L)
		return ReturnValue;

	FLuauValue Value = InTable.GetField(Key);
	if (Value.Type == ELuauValueType::Nil)
		return ReturnValue;

	Args.Insert(InTable, 0);

	return LuauValueCall(Value, Args);
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauTableIndexCall(FLuauValue InTable, int32 Index, TArray<FLuauValue> Args)
{
	FLuauValue ReturnValue;
	if (InTable.Type != ELuauValueType::Table)
		return ReturnValue;

	ULuauState* L = InTable.LuauState.Get();
	if (!L)
		return ReturnValue;

	FLuauValue Value = InTable.GetFieldByIndex(Index);
	if (Value.Type == ELuauValueType::Nil)
		return ReturnValue;

	return LuauValueCall(Value, Args);
}

TArray<FLuauValue> ULuauBlueprintFunctionLibrary::LuauTableUnpack(FLuauValue InTable)
{
	TArray<FLuauValue> ReturnValue;
	if (InTable.Type != ELuauValueType::Table)
		return ReturnValue;

	int32 Index = 1;
	for (;;)
	{
		FLuauValue Item = InTable.GetFieldByIndex(Index++);
		if (Item.Type == ELuauValueType::Nil)
			break;
		ReturnValue.Add(Item);
	}

	return ReturnValue;
}

TArray<FLuauValue> ULuauBlueprintFunctionLibrary::LuauTableMergeUnpack(FLuauValue InTable1, FLuauValue InTable2)
{
	TArray<FLuauValue> ReturnValue;
	if (InTable1.Type != ELuauValueType::Table)
		return ReturnValue;

	if (InTable2.Type != ELuauValueType::Table)
		return ReturnValue;

	int32 Index = 1;
	for (;;)
	{
		FLuauValue Item = InTable1.GetFieldByIndex(Index++);
		if (Item.Type == ELuauValueType::Nil)
			break;
		ReturnValue.Add(Item);
	}

	for (;;)
	{
		FLuauValue Item = InTable2.GetFieldByIndex(Index++);
		if (Item.Type == ELuauValueType::Nil)
			break;
		ReturnValue.Add(Item);
	}

	return ReturnValue;
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauTablePack(UObject* WorldContextObject, TSubclassOf<ULuauState> State, TArray<FLuauValue> Values)
{
	FLuauValue ReturnValue;
	ULuauState* L = FLuauMachineModule::Get().GetLuauState(State, WorldContextObject->GetWorld());
	if (!L)
		return ReturnValue;

	ReturnValue = L->CreateLuauTable();

	int32 Index = 1;

	for (FLuauValue& Value : Values)
	{
		ReturnValue.SetFieldByIndex(Index++, Value);
	}

	return ReturnValue;
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauTableMergePack(UObject* WorldContextObject, TSubclassOf<ULuauState> State, TArray<FLuauValue> Values1, TArray<FLuauValue> Values2)
{
	FLuauValue ReturnValue;
	ULuauState* L = FLuauMachineModule::Get().GetLuauState(State, WorldContextObject->GetWorld());
	if (!L)
		return ReturnValue;

	ReturnValue = L->CreateLuauTable();

	int32 Index = 1;

	for (FLuauValue& Value : Values1)
	{
		ReturnValue.SetFieldByIndex(Index++, Value);
	}

	for (FLuauValue& Value : Values2)
	{
		ReturnValue.SetFieldByIndex(Index++, Value);
	}

	return ReturnValue;
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauTableFromMap(UObject* WorldContextObject, TSubclassOf<ULuauState> State, TMap<FString, FLuauValue> Map)
{
	FLuauValue ReturnValue;
	ULuauState* L = FLuauMachineModule::Get().GetLuauState(State, WorldContextObject->GetWorld());
	if (!L)
		return ReturnValue;

	ReturnValue = L->CreateLuauTable();

	for (TPair<FString, FLuauValue>& Pair : Map)
	{
		ReturnValue.SetField(Pair.Key, Pair.Value);
	}

	return ReturnValue;
}

TArray<FLuauValue> ULuauBlueprintFunctionLibrary::LuauTableRange(FLuauValue InTable, int32 First, int32 Last)
{
	TArray<FLuauValue> ReturnValue;
	if (InTable.Type != ELuauValueType::Table)
		return ReturnValue;

	for (int32 i = First; i <= Last; i++)
	{
		ReturnValue.Add(InTable.GetFieldByIndex(i));
	}

	return ReturnValue;
}

TArray<FLuauValue> ULuauBlueprintFunctionLibrary::LuauValueArrayMerge(TArray<FLuauValue> Array1, TArray<FLuauValue> Array2)
{
	TArray<FLuauValue> NewArray = Array1;
	NewArray.Append(Array2);
	return NewArray;
}

TArray<FLuauValue> ULuauBlueprintFunctionLibrary::LuauValueArrayAppend(TArray<FLuauValue> Array, FLuauValue Value)
{
	TArray<FLuauValue> NewArray = Array;
	NewArray.Add(Value);
	return NewArray;
}

TArray<FLuauValue> ULuauBlueprintFunctionLibrary::LuauValueCallMulti(FLuauValue Value, TArray<FLuauValue> Args)
{
	TArray<FLuauValue> ReturnValue;

	ULuauState* L = Value.LuauState.Get();
	if (!L)
		return ReturnValue;

	L->FromLuauValue(Value);

	int32 StackTop = L->GetTop();

	int NArgs = 0;
	for (FLuauValue& Arg : Args)
	{
		L->FromLuauValue(Arg);
		NArgs++;
	}

	FLuauValue LastReturnValue;
	if (L->PCall(NArgs, LastReturnValue, LUA_MULTRET))
	{

		int32 NumOfReturnValues = (L->GetTop() - StackTop) + 1;
		if (NumOfReturnValues > 0)
		{
			for (int32 i = -1; i >= -(NumOfReturnValues); i--)
			{
				ReturnValue.Insert(L->ToLuauValue(i), 0);
			}
			L->Pop(NumOfReturnValues - 1);
		}

	}

	L->Pop();

	return ReturnValue;
}

void ULuauBlueprintFunctionLibrary::LuauValueYield(FLuauValue Value, TArray<FLuauValue> Args)
{
	if (Value.Type != ELuauValueType::Thread)
		return;

	ULuauState* L = Value.LuauState.Get();
	if (!L)
		return;

	L->FromLuauValue(Value);

	int32 StackTop = L->GetTop();

	int NArgs = 0;
	for (FLuauValue& Arg : Args)
	{
		L->FromLuauValue(Arg);
		NArgs++;
	}

	L->Yield(-1 - NArgs, NArgs);

	L->Pop();
}

TArray<FLuauValue> ULuauBlueprintFunctionLibrary::LuauValueResumeMulti(FLuauValue Value, TArray<FLuauValue> Args)
{
	TArray<FLuauValue> ReturnValue;

	if (Value.Type != ELuauValueType::Thread)
		return ReturnValue;

	ULuauState* L = Value.LuauState.Get();
	if (!L)
		return ReturnValue;

	L->FromLuauValue(Value);

	int32 StackTop = L->GetTop();

	int NArgs = 0;
	for (FLuauValue& Arg : Args)
	{
		L->FromLuauValue(Arg);
		NArgs++;
	}

	L->Resume(-1 - NArgs, NArgs);

	int32 NumOfReturnValues = (L->GetTop() - StackTop);
	if (NumOfReturnValues > 0)
	{
		for (int32 i = -1; i >= -(NumOfReturnValues); i--)
		{
			ReturnValue.Insert(L->ToLuauValue(i), 0);
		}
		L->Pop(NumOfReturnValues);
	}

	L->Pop();

	return ReturnValue;
}

FVector ULuauBlueprintFunctionLibrary::LuauTableToVector(FLuauValue Value)
{
	if (Value.Type != ELuauValueType::Table)
	{
		return FVector(NAN);
	}

	auto GetVectorField = [](FLuauValue& Table, const char* Field_n, const char* Field_N, int32 Index) -> FLuauValue
	{
		FLuauValue N = Table.GetField(Field_n);
		if (N.IsNil())
		{
			N = Table.GetField(Field_N);
			if (N.IsNil())
			{
				N = Table.GetFieldByIndex(Index);
				if (N.IsNil())
				{
					N = FLuauValue(NAN);
				}
			}
		}
		return N;
	};

	FLuauValue X = GetVectorField(Value, "x", "X", 1);
	FLuauValue Y = GetVectorField(Value, "y", "Y", 2);
	FLuauValue Z = GetVectorField(Value, "z", "Z", 3);

	return FVector(X.ToFloat(), Y.ToFloat(), Z.ToFloat());
}

FRotator ULuauBlueprintFunctionLibrary::LuauTableToRotator(FLuauValue Value)
{
	if (Value.Type != ELuauValueType::Table)
	{
		return FRotator(NAN);
	}

	auto GetRotatorField = [](FLuauValue& Table, const char* Field_n, const char* Field_N, int32 Index) -> FLuauValue
		{
			FLuauValue N = Table.GetField(Field_n);
			if (N.IsNil())
			{
				N = Table.GetField(Field_N);
				if (N.IsNil())
				{
					N = Table.GetFieldByIndex(Index);
					if (N.IsNil())
					{
						N = FLuauValue(NAN);
					}
				}
			}
			return N;
		};

	FLuauValue Roll = GetRotatorField(Value, "roll", "Roll", 1);
	FLuauValue Pitch = GetRotatorField(Value, "pitch", "Pitch", 2);
	FLuauValue Yaw = GetRotatorField(Value, "yaw", "Yaw", 3);

	return FRotator(Pitch.ToFloat(), Yaw.ToFloat(), Roll.ToFloat());
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauTableSetMetaTable(FLuauValue InTable, FLuauValue InMetaTable)
{
	FLuauValue ReturnValue;
	if (InTable.Type != ELuauValueType::Table || InMetaTable.Type != ELuauValueType::Table)
		return ReturnValue;

	ULuauState* L = InTable.LuauState.Get();
	if (!L)
		return ReturnValue;

	return InTable.SetMetaTable(InMetaTable);
}

int32 ULuauBlueprintFunctionLibrary::LuauValueLength(FLuauValue Value)
{

	ULuauState* L = Value.LuauState.Get();
	if (!L)
		return 0;

	L->FromLuauValue(Value);
	L->Len(-1);
	int32 Length = L->ToInteger(-1);
	L->Pop(2);

	return Length;
}

TArray<FLuauValue> ULuauBlueprintFunctionLibrary::LuauTableGetKeys(FLuauValue Table)
{
	TArray<FLuauValue> Keys;

	if (Table.Type != ELuauValueType::Table)
		return Keys;

	ULuauState* L = Table.LuauState.Get();
	if (!L)
		return Keys;

	L->FromLuauValue(Table);
	L->PushNil(); // first key
	while (L->Next(-2))
	{
		Keys.Add(L->ToLuauValue(-2)); // add key
		L->Pop(); // pop the value
	}

	L->Pop(); // pop the table

	return Keys;
}

TArray<FLuauValue> ULuauBlueprintFunctionLibrary::LuauTableGetValues(FLuauValue Table)
{
	TArray<FLuauValue> Keys;

	if (Table.Type != ELuauValueType::Table)
		return Keys;

	ULuauState* L = Table.LuauState.Get();
	if (!L)
		return Keys;

	L->FromLuauValue(Table);
	L->PushNil(); // first key
	while (L->Next(-2))
	{
		Keys.Add(L->ToLuauValue(-1)); // add value
		L->Pop(); // pop the value
	}

	L->Pop(); // pop the table

	return Keys;
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauTableAssetToLuauTable(UObject* WorldContextObject, TSubclassOf<ULuauState> State, ULuauTableAsset* TableAsset)
{
	ULuauState* L = FLuauMachineModule::Get().GetLuauState(State, WorldContextObject->GetWorld());
	if (!L)
		return FLuauValue();

	return TableAsset->ToLuauTable(L);
}

FLuauValue ULuauBlueprintFunctionLibrary::LuauNewLuauUserDataObject(UObject* WorldContextObject, TSubclassOf<ULuauState> State, TSubclassOf<ULuauUserDataObject> UserDataObjectClass, bool bTrackObject)
{
	ULuauState* L = FLuauMachineModule::Get().GetLuauState(State, WorldContextObject->GetWorld());
	if (!L)
		return FLuauValue();

	return L->NewLuauUserDataObject(UserDataObjectClass, bTrackObject);
}

ULuauState* ULuauBlueprintFunctionLibrary::LuauGetState(UObject* WorldContextObject, TSubclassOf<ULuauState> State)
{
	return FLuauMachineModule::Get().GetLuauState(State, WorldContextObject->GetWorld());
}

bool ULuauBlueprintFunctionLibrary::LuauTableImplements(FLuauValue Table, ULuauTableAsset* TableAsset)
{
	if (Table.Type != ELuauValueType::Table)
		return false;

	ULuauState* L = Table.LuauState.Get();
	if (!L)
		return false;

	for (TPair<FString, FLuauValue>& Pair : TableAsset->Table)
	{
		FLuauValue Item = Table.GetField(Pair.Key);
		if (Item.Type == ELuauValueType::Nil)
			return false;
		if (Item.Type != Pair.Value.Type)
			return false;
	}

	return true;
}

bool ULuauBlueprintFunctionLibrary::LuauTableImplementsAll(FLuauValue Table, TArray<ULuauTableAsset*> TableAssets)
{
	for (ULuauTableAsset* TableAsset : TableAssets)
	{
		if (!LuauTableImplements(Table, TableAsset))
			return false;
	}
	return true;
}

bool ULuauBlueprintFunctionLibrary::LuauTableImplementsAny(FLuauValue Table, TArray<ULuauTableAsset*> TableAssets)
{
	for (ULuauTableAsset* TableAsset : TableAssets)
	{
		if (LuauTableImplements(Table, TableAsset))
			return true;
	}
	return false;
}

int32 ULuauBlueprintFunctionLibrary::LuauGetUsedMemory(UObject* WorldContextObject, TSubclassOf<ULuauState> State)
{
	ULuauState* L = FLuauMachineModule::Get().GetLuauState(State, WorldContextObject->GetWorld());
	if (!L)
		return -1;

	return L->GC(LUA_GCCOUNT);
}

void ULuauBlueprintFunctionLibrary::LuauGCCollect(UObject* WorldContextObject, TSubclassOf<ULuauState> State)
{
	ULuauState* L = FLuauMachineModule::Get().GetLuauState(State, WorldContextObject->GetWorld());
	if (!L)
		return;

	L->GC(LUA_GCCOLLECT);
}

void ULuauBlueprintFunctionLibrary::LuauGCStop(UObject* WorldContextObject, TSubclassOf<ULuauState> State)
{
	ULuauState* L = FLuauMachineModule::Get().GetLuauState(State, WorldContextObject->GetWorld());
	if (!L)
		return;

	L->GC(LUA_GCSTOP);
}

void ULuauBlueprintFunctionLibrary::LuauGCRestart(UObject* WorldContextObject, TSubclassOf<ULuauState> State)
{
	ULuauState* L = FLuauMachineModule::Get().GetLuauState(State, WorldContextObject->GetWorld());
	if (!L)
		return;

	L->GC(LUA_GCRESTART);
}

void ULuauBlueprintFunctionLibrary::LuauSetUserDataMetaTable(UObject* WorldContextObject, TSubclassOf<ULuauState> State, FLuauValue MetaTable)
{
	ULuauState* L = FLuauMachineModule::Get().GetLuauState(State, WorldContextObject->GetWorld());
	if (!L)
		return;
	L->SetUserDataMetaTable(MetaTable);
}

bool ULuauBlueprintFunctionLibrary::LuauValueIsReferencedInLuauRegistry(FLuauValue Value)
{
	return Value.IsReferencedInLuauRegistry();
}

UClass* ULuauBlueprintFunctionLibrary::LuauValueToBlueprintGeneratedClass(const FLuauValue& Value)
{
	UObject* LoadedObject = nullptr;
	if (Value.Type == ELuauValueType::String)
	{
		LoadedObject = StaticLoadObject(UBlueprint::StaticClass(), nullptr, *Value.ToString());
	}
	else if (Value.Type == ELuauValueType::UObject)
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

UClass* ULuauBlueprintFunctionLibrary::LuauValueLoadClass(const FLuauValue& Value, bool bDetectBlueprintGeneratedClass)
{
	UObject* LoadedObject = nullptr;
	if (Value.Type == ELuauValueType::String)
	{
		LoadedObject = StaticLoadObject(UObject::StaticClass(), nullptr, *Value.ToString());
	}
	else if (Value.Type == ELuauValueType::UObject)
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

UObject* ULuauBlueprintFunctionLibrary::LuauValueLoadObject(const FLuauValue& Value)
{
	UObject* LoadedObject = nullptr;
	if (Value.Type == ELuauValueType::String)
	{
		LoadedObject = StaticLoadObject(UObject::StaticClass(), nullptr, *Value.ToString());
	}
	else if (Value.Type == ELuauValueType::UObject)
	{
		LoadedObject = Value.Object;
	}

	return LoadedObject;
}

bool ULuauBlueprintFunctionLibrary::LuauValueFromJson(UObject* WorldContextObject, TSubclassOf<ULuauState> State, const FString& Json, FLuauValue& LuauValue)
{
	// default to nil
	LuauValue = FLuauValue();

	ULuauState* L = FLuauMachineModule::Get().GetLuauState(State, WorldContextObject->GetWorld());
	if (!L)
		return false;

	TSharedPtr<FJsonValue> JsonValue;
	TSharedRef< TJsonReader<TCHAR> > JsonReader = TJsonReaderFactory<TCHAR>::Create(Json);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonValue))
	{
		return false;
	}

	LuauValue = FLuauValue::FromJsonValue(L, *JsonValue);
	return true;
}

FString ULuauBlueprintFunctionLibrary::LuauValueToJson(FLuauValue Value)
{
	FString Json;
	TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&Json);
	FJsonSerializer::Serialize(Value.ToJsonValue(), "", JsonWriter);
	return Json;
}

bool ULuauBlueprintFunctionLibrary::LuauLoadPakFile(const FString& Filename, FString Mountpoint, TArray<FLuauValue>& Assets, FString ContentPath, FString AssetRegistryPath)
{
	if (!Mountpoint.StartsWith("/") || !Mountpoint.EndsWith("/"))
	{
		UE_LOG(LogLuauMachine, Error, TEXT("Invalid Mountpoint, must be in the format /Name/"));
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
			UE_LOG(LogLuauMachine, Error, TEXT("Unable to setup PakPlatformFile"));
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
		UE_LOG(LogLuauMachine, Error, TEXT("Unable to open PakFile"));
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
		UE_LOG(LogLuauMachine, Error, TEXT("Unable to mount PakFile"));
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
		UE_LOG(LogLuauMachine, Error, TEXT("Unable to parse AssetRegistry file"));
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
			Assets.Add(FLuauValue(Asset.GetAsset()));
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

void ULuauBlueprintFunctionLibrary::SwitchOnLuauValueType(const FLuauValue& LuauValue, ELuauValueType& LuauValueTypes)
{
	LuauValueTypes = LuauValue.Type;
}

void ULuauBlueprintFunctionLibrary::GetLuauReflectionType(UObject* InObject, const FString& Name, ELuauReflectionType& LuauReflectionTypes)
{
	LuauReflectionTypes = ELuauReflectionType::Unknown;
	UClass* Class = InObject->GetClass();
	if (!Class)
	{
		return;
	}

	if (Class->FindPropertyByName(FName(*Name)) != nullptr)
	{
		LuauReflectionTypes = ELuauReflectionType::Property;
		return;
	}

	if (Class->FindFunctionByName(FName(*Name)))
	{
		LuauReflectionTypes = ELuauReflectionType::Function;
		return;
	}
}

void ULuauBlueprintFunctionLibrary::RegisterLuauConsoleCommand(const FString& CommandName, const FLuauValue& LuauConsoleCommand)
{
	FLuauMachineModule::Get().RegisterLuauConsoleCommand(CommandName, LuauConsoleCommand);
}

void ULuauBlueprintFunctionLibrary::UnregisterLuauConsoleCommand(const FString& CommandName)
{
	FLuauMachineModule::Get().UnregisterLuauConsoleCommand(CommandName);
}

ULuauState* ULuauBlueprintFunctionLibrary::CreateDynamicLuauState(UObject* WorldContextObject, TSubclassOf<ULuauState> LuauStateClass)
{
	if (!LuauStateClass)
	{
		return nullptr;
	}

	if (LuauStateClass == ULuauState::StaticClass())
	{
		UE_LOG(LogLuauMachine, Error, TEXT("attempt to use LuauState Abstract class, please create a child of LuauState"));
		return nullptr;
	}


	ULuauState* NewLuauState = NewObject<ULuauState>((UObject*)GetTransientPackage(), LuauStateClass);
	if (!NewLuauState)
	{
		return nullptr;
	}

	return NewLuauState->GetLuauState(WorldContextObject->GetWorld());
}
