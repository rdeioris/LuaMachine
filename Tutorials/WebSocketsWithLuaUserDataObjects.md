# Implementing a WebSockets API using LuaUserDataObjects

## LuaUserDataObjects ???

The 'ULuaUserDataObject' UCLASS allows you to create new kinds of LuaValues built over the UObject system.

In addition to 'primitive' types (string, boolean, numbers, tables ...) Lua allows for so called "userdata". At the lowest level they are pointers to custom data with (eventually) a metatable
attached to them. Having a metatable allows them to expose parameters/methods like normal tables.

If you want to expose methods or attributes for LuaValues wrapping UObjects, you can create a new UnrealEngine class (Blueprint or C++) inheriting from ULuaUserDataObject and add items to the Table and Metatable properties of the object
(like with LuaStates, LuaBlueprintPackages and LuaComponents).

Note that is not responsability of the Lua VM to manage the lifetime of the associated ULuaUserDataObject: if the Unreal Garbage Collector frees it, the FLuaValue will become nil.

## The ULuaWebSocketConnection UCLASS

The objective is to define a new lua type representing a WebSocket connection to a url exposing a bunch of methods (connect, disconnect, send and close) and attributes (on_message, on_closed).

We start by subclassing UluaUserDataObject: 

```c++
#pragma once

#include "CoreMinimal.h"
#include "LuaUserDataObject.h"
#include "IWebSocket.h"
#include "LuaWebsocketConnection.generated.h"

/**
 * 
 */
UCLASS()
class RPGLUA_API ULuaWebsocketConnection : public ULuaUserDataObject
{
	GENERATED_BODY()

public:
	ULuaWebsocketConnection();
	~ULuaWebsocketConnection();

	UFUNCTION()
	void Connect(FLuaValue Url, FLuaValue InOnConnectedCallback, FLuaValue InOnConnectionErrorCallback);

	UFUNCTION()
	void Disconnect();

	UFUNCTION()
	void Send(FLuaValue Message);

	UFUNCTION()
	FLuaValue ToLuaString();

	UFUNCTION()
	void Close(FLuaValue Code, FLuaValue Reason);

protected:
	TSharedPtr<IWebSocket> WebSocketConnection;
	TSharedPtr<FLuaSmartReference> OnConnectedCallback;
	TSharedPtr<FLuaSmartReference> OnConnectionErrorCallback;

	void OnMessageDelegate(const FString& Message);
	void OnClosedDelegate(int32 Code, const FString& Reason, bool bUserClose);

	void OnConnectedDelegate(TWeakPtr<FLuaSmartReference> OnConnectedCallbackRef);
	void OnConnectionErrorDelegate(const FString& Message, TWeakPtr<FLuaSmartReference> OnConnectionErrorCallbackRef);

	FString CurrentUrl;
};
```
