# Using the HTTP + JSON Functions with RickAndMorty API

LuaMachine includes out of the box UFUNCTION's for interfacing with HTTP services.

In this tutorial we will see how to retrieve JSON data as well as raw images/textures from an HTTP service.

The objective is to build a User Widget listing all of the Rick And Morty characters available at https://rickandmortyapi.com/

![RickAndMortyAPI](RickAndMortyAPI_Data/RickAndMorty.PNG?raw=true "RickAndMortyAPI")

## Step 1: adding http_get() and from_json() lua functions

By default a LuaMachine state has no function exposed to the user/scripter. You generally want to carefully choose what to expose to the final scripter/modder, so the default behaviour
is to expose nothing (except the lua standard functions [and this is configurable too]).

We want to expose two functions: one for making http requests (we will call it http_get()) and another for converting json strings to lua tables (we will call it from_json())

Let's add a new LuaState called RickAndMortyLuaState:

![RickAndMortyNewLuaState](RickAndMortyAPI_Data/RickAndMorty001.PNG?raw=true "RickAndMortyNewLuaState")

![RickAndMortyNewLuaStateContentBrowser](RickAndMortyAPI_Data/RickAndMorty002.PNG?raw=true "RickAndMortyNewLuaStateContentBrowser")

And add a new Custom Event (note: not a plain function!) named HttpGet:

![RickAndMortyHttpGet](RickAndMortyAPI_Data/RickAndMorty003.PNG?raw=true "RickAndMortyHttpGet")

As you can see the event will trigger the HTTP request, and two additional events are generated (one for successful connection, the other for failures) mapped to lua callbacks.

Now expose the event as a lua function:

![RickAndMortyHttpGetTable](RickAndMortyAPI_Data/RickAndMorty004.PNG?raw=true "RickAndMortyHttpGetTable")

And create a Lua Code Asset for working with http_get():

```lua
http_get('https://rickandmortyapi.com/api/character',
  function(status, headers, content, data)
    print(status)
    print(content)
  end,
  function(data)
    error('unable to connect to http service')
  end
)
```

The successful callback will get status, headers, content and a context object (nil for now), while the error one will only get the context.

Now in your level blueprint add a node for calling the Lua Code Asset, play the level and check Unreal logs (they should show both http status and http response):

![RickAndMortyLevelBlueprint](RickAndMortyAPI_Data/RickAndMorty010.PNG?raw=true "RickAndMortyLevelBlueprint")

Time to add the from_json() function. This time is a standard UFUNCTION (not a custom event like http_get()):

![RickAndMortyFromJSON](RickAndMortyAPI_Data/RickAndMorty009.PNG?raw=true "RickAndMortyFromJSON")

Add from_json() to the Lua State Table and update the Lua Code Asset for parsing the json response:

```lua
http_get('https://rickandmortyapi.com/api/character',
  function(status, headers, content, data)
    print(status)
    foo = from_json(content)
  end,
  function(data)
    error('unable to connect to http service')
  end
)
```

Before hitting play again, open the Lua Machine Debugger (it is under the Window/Developer Tools menu). When the level runs hit "refresh" in the debugger and you should see the 'foo' object tree:

![RickAndMortyLuaDebugger](RickAndMortyAPI_Data/RickAndMorty005.PNG?raw=true "RickAndMortyLuaDebugger")

## Step 2: adding a LuaUserDataObject representing a RickAndMorty character

In addition to Lua native types (strings, integers, booleans, tables...), LuaMachine allows you to define new types based on UObjects.

We will now create a new Lua UserData type for representing a RickAndMorty Character:

![RickAndMortyUserData](RickAndMortyAPI_Data/RickAndMorty006.PNG?raw=true "RickAndMortyUserData")

This new UObject includes a variable named "Image" of type SlateBrush (will show the character image/texture retrieved via HTTP), a UFUNCTION SetImage (exposed as a lua method named set_image()) and 4 pure UFUNCTIONS for retrieving specific fields:


![RickAndMortyLuaUserDataObject](RickAndMortyAPI_Data/RickAndMorty012.PNG?raw=true "RickAndMortyLuaUserDataObject")

Note that you have dozens of different ways to expose the UObject fields to the other systems, adding pure functions is only one of them.
As this object needs to be accessed by the widget subsystem, this approach reduces the amount of nodes in the widget itself.

Time to add a new 'character()" function to the LuaState to create new lua userdata characters:

![RickAndMortyNewCharacter](RickAndMortyAPI_Data/RickAndMorty007.PNG?raw=true "RickAndMortyNewCharacter")

![RickAndMortyNewCharacterTable](RickAndMortyAPI_Data/RickAndMorty008.PNG?raw=true "RickAndMortyNewCharacterTable")

Time to refactor the code to create a character() for each result returned by the HTTP api and to download the images/textures by issuing another http_get() call:

```lua
http_get('https://rickandmortyapi.com/api/character',
  function(status, headers, content, data)
    if status ~= 200 then
      error('HTTP error code: ' .. status)
    end
    local response = from_json(content)
    for _, result in ipairs(response.results) do
      local new_character = character()
      new_character.name = result.name
      new_character.gender = result.gender
      new_character.status = result.status
      new_character.species = result.species
      http_get(result.image,
        function(status, headers, content, data)
          if status ~= 200 then
            error('HTTP error code for image: ' .. status)
          end
          local current_character = data
          current_character.set_image(content)
        end,
        function(data)
          error('unable to connect to http image service')
        end,
        new_character
      )
    end
  end,
  function(data)
    error('unable to connect to http service')
  end
)
```

## Step 3: creating the widgets

The Lua Virtual Machine has now a bunch of 'lua character' objects hanging around just waiting to be visualized.

Create a new User Widget containing a "List View" (the 'CharactersList' in the screenshot):

![RickAndMortyListView](RickAndMortyAPI_Data/RickAndMorty013.PNG?raw=true "RickAndMortyListView")

List Widgets require "Entry Widgets". They are UserWidget implementing the "User List Entry" interface. You can create
the Entry Widget for the List View by clicking on the "+" button of the "Entry Widget Class" property:

![RickAndMortyEntryWidget](RickAndMortyAPI_Data/RickAndMorty015.PNG?raw=true "RickAndMortyEntryWidget")

We now need to add an "Add Character" function to the main widget (not the entry widget!):

![RickAndMortyAddCharacter](RickAndMortyAPI_Data/RickAndMorty016.PNG?raw=true "RickAndMortyAddCharacter")

This function will populate the internal list of the "List View" widget, and will generate "Entry Widgets" on-demand.

We are adding the Lua UserData Objects we created before as the list item. Our next task is to view them in the "Entry Widget".

The "Entry Widget" (the one implementing the "User List Entry" interface) has one image widget and 4 text widgets, each of them exposing one of the Lua UserData Object fields:

![RickAndMortyEntryWidgetDesign](RickAndMortyAPI_Data/RickAndMorty018.PNG?raw=true "RickAndMortyEntryWidgetDesign")

In the Widget Editor Graph section add a variable of the Lua UserData Character type:

![RickAndMortyEntryWidgetLua](RickAndMortyAPI_Data/RickAndMorty021.PNG?raw=true "RickAndMortyEntryWidgetLua")

This is the object exposing values to the various sub-widgets (the image and the 4 text boxes).

Now you can configure the image widget to read the 'Image' property from the Character User Data:

![RickAndMortyEntryWidgetImage](RickAndMortyAPI_Data/RickAndMorty019.PNG?raw=true "RickAndMortyEntryWidgetImage")

And specify the "pure function" to call for each text block:

![RickAndMortyEntryWidgetTextBlock](RickAndMortyAPI_Data/RickAndMorty020.PNG?raw=true "RickAndMortyEntryWidgetTextBlock")

Once compiled, you should see in the main widget preview something like this:

![RickAndMortyWidgetPreview](RickAndMortyAPI_Data/RickAndMorty014.PNG?raw=true "RickAndMortyWidgetPreview")

Last step is setting the Lua UserData Character field in the Entry widget whenever a new Entry widget is generated by the list view (by overriding the "On Entry Initialied" event:

![RickAndMortyOnEntryInitialized](RickAndMortyAPI_Data/RickAndMorty017.PNG?raw=true "RickAndMortyOnEntryInitialized")

## Step 4: placing all together

Now we need to improve the 'character()' lua function to call the 'AddCharacter' method of the main widget. For doing this we are adding a reference to the main widget in the lua state (as a variable):

![RickAndMortyNewCharacter2](RickAndMortyAPI_Data/RickAndMorty022.PNG?raw=true "RickAndMortyNewCharacter2")

Finally, the level blueprint will be modified for generating the widget and passing its reference to the LuaState:

![RickAndMortyLevelBlueprint2](RickAndMortyAPI_Data/RickAndMorty011.PNG?raw=true "RickAndMortyLevelBlueprint2")

## Step 5: pagination
The RickAndMorty API is based on 'pages'. Currently we are only retrieving the page 1.

For each page the api exposes the info.prev and info.next fields containing the urls of the previous page (if available) and the next page (if available).

Let's refactor the code again:

```lua

function load_page_from_url(url)

  http_get(url,
    function(status, headers, content, data)
      if status ~= 200 then
        error('HTTP error code: ' .. status)
      end
      local response = from_json(content)
      prev_page_url = response.info.prev
      next_page_url = response.info.next
      for _, result in ipairs(response.results) do
        local new_character = character()
        new_character.name = result.name
        new_character.gender = result.gender
        new_character.status = result.status
        new_character.species = result.species
        http_get(result.image,
          function(status, headers, content, data)
            if status ~= 200 then
              error('HTTP error code for image: ' .. status)
            end
            local current_character = data
            current_character.set_image(content)
          end,
          function(data)
            error('unable to connect to http image service')
          end,
          new_character
        )
      end
    end,
    function(data)
      error('unable to connect to http service')
    end
  )

end

load_page_from_url('https://rickandmortyapi.com/api/character')
```

The 'load_page_from_url()' function, now takes an arbitrary url.

Let's add two buttons to the main widget and set their 'click event':

![RickAndMortyPrevNext](RickAndMortyAPI_Data/RickAndMorty023.PNG?raw=true "RickAndMortyPrevNext")

![RickAndMortyPrevNextClick](RickAndMortyAPI_Data/RickAndMorty024.PNG?raw=true "RickAndMortyPrevNextClick")

Note that we are using two different approaches: in the 'prev' case we retrieve each lua object (function and url) and then we call it. This approach is way more elegant than the 'next' one (where we simply invoke a lua string) but obviously requires more nodes.

Clicking on 'next' will show the next api page (after having cleared the list of items):

![RickAndMortyPage2](RickAndMortyAPI_Data/RickAndMorty025.PNG?raw=true "RickAndMortyPage2")

## Optional Step 6: using C++

While it is pretty easy to define lua apis from blueprints, using C++ is way faster and less verbose. This is the equivalent C++ code for the previously defined LuaState as well as the Lua UserData Object:

### RickAndMortyLuaStateBase.h
```cpp
#pragma once

#include "CoreMinimal.h"
#include "LuaState.h"
#include "RickAndMortyLuaStateBase.generated.h"

/**
 * 
 */
UCLASS()
class LUARICKANDMORTY_API URickAndMortyLuaStateBase : public ULuaState
{
	GENERATED_BODY()

public:
	URickAndMortyLuaStateBase();

	UFUNCTION()
	void HttpGet(FLuaValue Url, FLuaValue SuccessCallback, FLuaValue ErrorCallback, FLuaValue Data);
	
	UFUNCTION()
	void HttpSuccessCallback(FLuaValue Context, FLuaValue Response);

	UFUNCTION()
	void HttpErrorCallback(FLuaValue Data);

	UFUNCTION()
	bool FromJSON(FLuaValue Data, FLuaValue& Value);

	UFUNCTION()
	FLuaValue NewCharacter();
};

```

### RickAndMortyCharacterBase.h
```cpp

#pragma once

#include "CoreMinimal.h"
#include "LuaUserDataObject.h"
#include "Styling/SlateBrush.h"
#include "RickAndMortyCharacterBase.generated.h"

/**
 * 
 */
UCLASS()
class LUARICKANDMORTY_API URickAndMortyCharacterBase : public ULuaUserDataObject
{
	GENERATED_BODY()

public:
	URickAndMortyCharacterBase();

	UPROPERTY(EditAnywhere)
	FSlateBrush Image;

	UFUNCTION()
	void SetImage(FLuaValue Data);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FText GetName();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FText GetStatus();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FText GetGender();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FText GetSpecies();

};

```

### RickAndMortyLuaStateBase.cpp
```cpp

#include "RickAndMortyLuaStateBase.h"
#include "LuaBlueprintFunctionLibrary.h"
#include "RickAndMortyCharacterBase.h"

URickAndMortyLuaStateBase::URickAndMortyLuaStateBase()
{
	Table.Add("http_get", FLuaValue::Function(GET_FUNCTION_NAME_CHECKED(URickAndMortyLuaStateBase, HttpGet)));
	Table.Add("from_json", FLuaValue::Function(GET_FUNCTION_NAME_CHECKED(URickAndMortyLuaStateBase, FromJSON)));
	Table.Add("character", FLuaValue::Function(GET_FUNCTION_NAME_CHECKED(URickAndMortyLuaStateBase, NewCharacter)));
}

void URickAndMortyLuaStateBase::HttpGet(FLuaValue Url, FLuaValue SuccessCallback, FLuaValue ErrorCallback, FLuaValue Data)
{
	FLuaValue Context = CreateLuaTable();
	Context.SetFieldByIndex(1, SuccessCallback);
	Context.SetFieldByIndex(2, ErrorCallback);
	Context.SetFieldByIndex(3, Data);

	FLuaHttpResponseReceived ResponseReceived;
	ResponseReceived.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(URickAndMortyLuaStateBase, HttpSuccessCallback));

	FLuaHttpError ErrorReceived;
	ErrorReceived.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(URickAndMortyLuaStateBase, HttpErrorCallback));

	ULuaBlueprintFunctionLibrary::LuaHttpRequest(GetWorld(), GetClass(), "GET", Url.ToString(), TMap<FString, FString>(), FLuaValue(), Context, ResponseReceived, ErrorReceived);
}

void URickAndMortyLuaStateBase::HttpSuccessCallback(FLuaValue Context, FLuaValue Response)
{
	FLuaValue Callback = Context.GetFieldByIndex(1);
	FLuaValue StatusCode = Response.GetFieldByIndex(1);
	FLuaValue Headers = Response.GetFieldByIndex(2);
	FLuaValue Content = Response.GetFieldByIndex(3);
	TArray<FLuaValue> Args = { StatusCode, Headers, Content, Context.GetFieldByIndex(3) };
	ULuaBlueprintFunctionLibrary::LuaValueCallIfNotNil(Callback, Args);
}

void URickAndMortyLuaStateBase::HttpErrorCallback(FLuaValue Data)
{
	FLuaValue Callback = Data.GetFieldByIndex(2);
	TArray<FLuaValue> Args = { Data.GetFieldByIndex(3) };
	ULuaBlueprintFunctionLibrary::LuaValueCallIfNotNil(Callback, Args);
}

bool URickAndMortyLuaStateBase::FromJSON(FLuaValue Data, FLuaValue& Value)
{
	return ULuaBlueprintFunctionLibrary::LuaValueFromJson(GetWorld(), GetClass(), Data.ToString(), Value);
}

FLuaValue URickAndMortyLuaStateBase::NewCharacter()
{
	return NewLuaUserDataObject(URickAndMortyCharacterBase::StaticClass());
}
```

### RickAndMortyCharacterBase.cpp
```cpp

#include "RickAndMortyCharacterBase.h"
#include "LuaBlueprintFunctionLibrary.h"
#include "Engine/Texture2D.h"

URickAndMortyCharacterBase::URickAndMortyCharacterBase()
{
	Table.Add("set_image", FLuaValue::Function(GET_FUNCTION_NAME_CHECKED(URickAndMortyCharacterBase, SetImage)));
}

void URickAndMortyCharacterBase::SetImage(FLuaValue Data)
{
	UTexture2D* Texture = ULuaBlueprintFunctionLibrary::LuaValueToTransientTexture(0, 0, Data, PF_B8G8R8A8, true);
	if (Texture)
	{
		Image.DrawAs = ESlateBrushDrawType::Image;
		Image.ImageSize = FVector2D(200, 200);
		Image.SetResourceObject(Texture);
	}
}

FText URickAndMortyCharacterBase::GetName()
{
	return FText::FromString(LuaGetField("name").ToString());
}

FText URickAndMortyCharacterBase::GetGender()
{
	return FText::FromString(LuaGetField("gender").ToString());
}

FText URickAndMortyCharacterBase::GetSpecies()
{
	return FText::FromString(LuaGetField("species").ToString());
}

FText URickAndMortyCharacterBase::GetStatus()
{
	return FText::FromString(LuaGetField("status").ToString());
}
```
