# Implementing a WebSockets API using LuaUserDataObjects

## LuaUserDataObjects ?

The 'ULuaUserDataObject' UCLASS allows you to create new kinds of LuaValues built over the UObject system.

In addition to 'primitive' types (string, boolean, numbers, tables ...) Lua allows for so called "userdata". At the lowest level they are pointers to custom data with (eventually) a metatable
attached to them. Having a metatable allows them to expose parameters/methods like normal tables.

If you want to expose methods or attributes for LuaValues wrapping UObjects, you can create a new Unreal Engine UCLASS (Blueprint or C++) inheriting from ULuaUserDataObject and add items to its Table and Metatable properties
(like with LuaStates, LuaBlueprintPackages and LuaComponents).


The following code shows what we are trying to accomplish:

```lua
local ws = websocket()

-- internally print() will call the __tostring metamethod of the ws object
print(ws)

-- this will be called whenever a new message is received
function ws:on_message(message)
  print(message)
  --ws:close(17, 'End of the World')
end

-- this is called on connection close
function ws:on_closed(code, reason, user)
  print('websocket connection closed: ' .. reason)
end

-- the first function/callback is called on successful connection
-- the second one on failure
ws:connect('wss://echo.websocket.org', function(self)
  print('connected')
  print(self)
end, function(self, reason)
  print('unable to connect: ' .. reason)
end)

-- this can be called from any part of your project to trigger sending of messages
function send_event(message)
  print('sending...')
  ws:send(message)
end
```

'Note' that is not responsibility of the Lua VM to manage the lifetime of the associated ULuaUserDataObject: if the Unreal Garbage Collector frees it, the FLuaValue will become nil.

## The ULuaWebSocketConnection UCLASS

The objective is to define a new lua type representing a WebSocket connection to a url exposing a bunch of methods (connect, disconnect, send and close) and attributes/callbacks (on_message, on_closed).

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

The 4 UFUNCTIONs will be exposed as lua functions (see the constructor below) while the two `TSharedPtr<FLuaSmartReference>` members are required for the OnConnectedDelegate and OnConnectionErrorDelegate functions.

Delegates can be triggered at any moment, even after the related FLuaValues (callback/functions) are destroyed. The FLuaSmartReference object allows you to link a LuaValue to a LuaState, avoiding it to be garbaged (note: remember to unlink them !).

The OnMessageDelegate and OnClosedDelegate do not require a FLuaSmartReference as they are based on attributes of the object itself (see ws:on_message() and ws:on_closed() functions defined in the previous lua code).

Time for functions definitions:

```c++
#include "LuaWebsocketConnection.h"
#include "LuaBlueprintFunctionLibrary.h"
#include "WebSocketsModule.h"

ULuaWebsocketConnection::ULuaWebsocketConnection()
{
	Table.Add("connect", FLuaValue::Function(GET_FUNCTION_NAME_CHECKED(ULuaWebsocketConnection, Connect)));
	Table.Add("disconnect", FLuaValue::Function(GET_FUNCTION_NAME_CHECKED(ULuaWebsocketConnection, Disconnect)));
	Table.Add("send", FLuaValue::Function(GET_FUNCTION_NAME_CHECKED(ULuaWebsocketConnection, Send)));
	Table.Add("close", FLuaValue::Function(GET_FUNCTION_NAME_CHECKED(ULuaWebsocketConnection, Close)));

	Metatable.Add("__tostring", FLuaValue::Function(GET_FUNCTION_NAME_CHECKED(ULuaWebsocketConnection, ToLuaString)));

	bImplicitSelf = true;
}

ULuaWebsocketConnection::~ULuaWebsocketConnection()
{
	Disconnect();
	UE_LOG(LogLuaMachine, Log, TEXT("Websocket Destroyed"));
}

void ULuaWebsocketConnection::Connect(FLuaValue Url, FLuaValue InOnConnectedCallback, FLuaValue InOnConnectionErrorCallback)
{
	Disconnect();

	CurrentUrl = Url.ToString();

	WebSocketConnection = FWebSocketsModule::Get().CreateWebSocket(Url.ToString(), TEXT("ws"));
	WebSocketConnection->OnMessage().AddUObject(this, &ULuaWebsocketConnection::OnMessageDelegate);
	WebSocketConnection->OnClosed().AddUObject(this, &ULuaWebsocketConnection::OnClosedDelegate);

	OnConnectedCallback = AddLuaSmartReference(InOnConnectedCallback);
	OnConnectionErrorCallback = AddLuaSmartReference(InOnConnectionErrorCallback);

	WebSocketConnection->OnConnectionError().AddUObject(this, &ULuaWebsocketConnection::OnConnectionErrorDelegate, TWeakPtr<FLuaSmartReference>(OnConnectionErrorCallback));
	WebSocketConnection->OnConnected().AddUObject(this, &ULuaWebsocketConnection::OnConnectedDelegate, TWeakPtr<FLuaSmartReference>(OnConnectedCallback));
	WebSocketConnection->Connect();
}

void ULuaWebsocketConnection::Disconnect()
{
	OnConnectedCallback.Reset();
	OnConnectionErrorCallback.Reset();

	if (WebSocketConnection.IsValid())
	{
		WebSocketConnection->Close();
		WebSocketConnection.Reset();
	}
}

void ULuaWebsocketConnection::Send(FLuaValue Message)
{
	if (WebSocketConnection.IsValid() && WebSocketConnection->IsConnected())
	{
		WebSocketConnection->Send(Message.ToString());
	}
}

void ULuaWebsocketConnection::Close(FLuaValue Code, FLuaValue Reason)
{
	if (WebSocketConnection.IsValid() && WebSocketConnection->IsConnected())
	{
		WebSocketConnection->Close(Code.Integer, Reason.ToString());
	}
}

FLuaValue ULuaWebsocketConnection::ToLuaString()
{
	if (WebSocketConnection.IsValid() && WebSocketConnection->IsConnected())
	{
		return FLuaValue(FString::Printf(TEXT("WebSocket Connected to %s"), *CurrentUrl));
	}
	return FLuaValue("Unconnected WebSocket");
}

void ULuaWebsocketConnection::OnMessageDelegate(const FString& Message)
{
	FLuaValue Callback = LuaGetField("on_message");
	TArray<FLuaValue> Args = { FLuaValue(this), FLuaValue(Message) };
	ULuaBlueprintFunctionLibrary::LuaValueCallIfNotNil(Callback, Args);
}

void ULuaWebsocketConnection::OnClosedDelegate(int32 Code, const FString& Reason, bool bUserClose)
{
	FLuaValue Callback = LuaGetField("on_closed");
	TArray<FLuaValue> Args = { FLuaValue(this), FLuaValue(Code), FLuaValue(Reason), FLuaValue(bUserClose) };
	ULuaBlueprintFunctionLibrary::LuaValueCallIfNotNil(Callback, Args);
}

void ULuaWebsocketConnection::OnConnectedDelegate(TWeakPtr<FLuaSmartReference> OnConnectedCallbackRef)
{
	if (!OnConnectedCallbackRef.IsValid())
		return;

	TArray<FLuaValue> Args = { FLuaValue(this) };
	ULuaBlueprintFunctionLibrary::LuaValueCallIfNotNil(OnConnectedCallbackRef.Pin()->Value, Args);
}

void ULuaWebsocketConnection::OnConnectionErrorDelegate(const FString& Message, TWeakPtr<FLuaSmartReference> OnConnectionErrorCallbackRef)
{
	if (!OnConnectionErrorCallbackRef.IsValid())
		return;

	TArray<FLuaValue> Args = { FLuaValue(this), FLuaValue(Message) };
	ULuaBlueprintFunctionLibrary::LuaValueCallIfNotNil(OnConnectionErrorCallbackRef.Pin()->Value, Args);
}
```

Note the checks for .IsValid() for the OnConnectedDelegate() and OnConnectionErrorDelegate() functions: they can be called at any moment, so we must be sure the native lua callbacks/functions are still valid. This is not required for OnMessageDelegate() and OnClosedDelegate() as they make use of the LuaGetField() function to retrieve the related lua callback.

Here the destructor prints a log message to allow the user/developer to track unexpected deallocations (see below)

## The LuaState

This is a simple LuaState showing how to exposed the previously create ULuaUserDataObject:

```c++
#pragma once

#include "CoreMinimal.h"
#include "LuaState.h"
#include "LuaWebsocketConnection.h"
#include "WebSocketsLuaStateBase.generated.h"

/**
 * 
 */
UCLASS()
class RPGLUA_API UWebSocketsLuaStateBase : public ULuaState
{
	GENERATED_BODY()

public:
	UWebSocketsLuaStateBase();

	UFUNCTION()
	FLuaValue GetWebSocketConnectionSingleton();

protected:
	UPROPERTY()
	ULuaWebsocketConnection* WebSocketConnectionSingleton;
	
};

```

The WebSocketConnectionSingleton member allows you to have a single instance of the websocket() object and, in addition to this, it will avoid the Unreal Garbage Collector to destroy it. If you want to allow the user/scripter to create more instances just use a TArray for tracking them (but implement a way to "untrack" them or to limit the amount of instances).

Let's go to the implementation:

```c++
#include "WebSocketsLuaStateBase.h"

UWebSocketsLuaStateBase::UWebSocketsLuaStateBase()
{
	Table.Add("websocket", FLuaValue::Function(GET_FUNCTION_NAME_CHECKED(UWebSocketsLuaStateBase, GetWebSocketConnectionSingleton)));
}

FLuaValue UWebSocketsLuaStateBase::GetWebSocketConnectionSingleton()
{
	if (!WebSocketConnectionSingleton)
	{
		WebSocketConnectionSingleton = NewObject<ULuaWebsocketConnection>(this);
	}
	return FLuaValue(WebSocketConnectionSingleton);
}
```

