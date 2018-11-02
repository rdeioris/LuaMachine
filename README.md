# LuaMachine

Unreal Engine 4 Plugin for adding Lua scripting to your projects.

If you want modders to customize your game/project, or you need to allow game designers to script parts of the logic, this plugin is for you.

## How it works

Contrary to other available Unreal Engine Lua plugins, this one does not try to expose the Unreal Engine 4 api, but completely hides it exposing to the user/scripter only the features the developer decided to include (via Blueprints or C++).

Once installed, the plugin will expose a "LuaState" UObject and a "LuaValue" UStruct, as well as a Blueprint Function Library and a handy Lua Component for mapping Actors to Lua objects.

### LuaState

A LuaState (ULuaState C++ class) represents a single Lua virtual machine (there can be multiple, each one isolated from the others). This is a "singleton" as each LuaState class can have a single instance for the whole process life.

Having multiple LuaState's allows clean separation of domains: as an example you may want a state for game configuration, another one for game logic and one for the modders. More complex examples include having states dedicated to specific Actors, GameModes or Levels.

LuaState's load and execute the lua code of your project. You can load scripts (both in text and bytecode format) from your filesystem, or from a specific asset exposed by the plugin, named LuaCode. The advantage of using filesystem scripts is that you can change them even after the project has been packaged, while LuaCode assets are directly built in your pak files.

Each LuaState can use both files and LuaCode assets.

Let's start with a simple example. Create a new LuaCode asset and edit it adding the following code:

```lua
dummyinteger = 17
dummystring = 'Hello World'
```

From the Lua point of view, "dummyinteger" and "dummystring" are fields of the global table (the one you can access from lua with the "_G" symbol)

Now create a LuaState and assign it the LuaCode asset.

Open your Level Blueprint and add the following nodes:

"LuaGetGlobal" will retrieve the value associated with the specified name

### LuaValue

LuaValue's are the way Unreal communicates with a specific Lua virtual machine. They contains values that both Lua and your project can use.

This a list of the currently supported values:

* Nil (nil/NULL value)
* Bool (boolean value)
* Integer (integer/int32 value)
* Number (float value)
* String (string/FString value)
* Function (reference to a lua function)
* Table (reference to a lua table)
* UObject (reference to an Unreal UObject)
* UFunction (reference to an Unreal UFunction)









