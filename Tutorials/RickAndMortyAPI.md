# Using the HTTP + JSON Functions with RickAndMorty API

LuaMachine includes out of the box UFUNCTION's for interfacing with HTTP services.

In this tututorial we will see how to retrieve JSON data as well as raw images/textures from an HTTP service.

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

As you can see the event will trigger the HTTP request, and two additional events are generated (one for successfull connection, the other for failures) mapped to lua callbacks.

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

The successfull callback will get status, headers, content and a context object (nil for now), while the error one will only get the context.

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

This new UObject includes a variable named "Image" of type SlateBrush (will show the character image/texture retrieved via HTTP), a UFUNCTION SetImage (exposed as a lua method named set_image()) and 4 pure UFUNCTIONS for retrieving specifing fields:


![RickAndMortyLuaUserDataObject](RickAndMortyAPI_Data/RickAndMorty012.PNG?raw=true "RickAndMortyLuaUserDataObject")

Note that you have dozens of different ways to expose the UObject fields to the other systems, adding pure functions is only one of them.
As this object needs to be accessed by the widget subsystem, this approach reduce the amount of nodes in the widget itself.

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

## Step 4: retrieving a characters page from the api

## Step 5: adding character's image

## Step 6: pagination

## Step 7: using C++
