# LuaComponent

This is an ActorComponent allowing you to expose Actor's related data to the Lua Virtual Machine.

The Component itself will be mapped to a lua userdata on a specific LuaState with its virtual table (and metatable)

![LuaComponent](Screenshots/LuaComponent.PNG?raw=true "LuaComponent")

Create a new lua script with the following content:

```lua
actor = {}

function actor:get_hp()
    self.name = 'test'
    print(self)
    self.hp = self.hp + 1
    return self.hp
end
```

Note the colon-syntax for implementing OOP style programming. Basically the LuaComponent will automatically pass the userdata as the first argument of actor.get_hp:

![LuaComponent2](Screenshots/LuaComponent2.PNG?raw=true "LuaComponent2")

While the game runs, you can see the "Table" property being updated from lua:

![LuaComponent3](Screenshots/LuaComponent3.PNG?raw=true "LuaComponent3")

## Metatable

You can specify a metatable for the LuaComponent. As an example you may want to implement a "__tostring" function so when doing

```lua
print(self)
```

you can get a meaningful value:

![LuaComponent4](Screenshots/LuaComponent4.PNG?raw=true "LuaComponent4")

![LuaComponent5](Screenshots/LuaComponent5.PNG?raw=true "LuaComponent5")

## LuaComponent Interactions

When passing UObject's as LuaValue, they are incapsulated as userdata, so technically (by default) you cannot do anything relevant via Lua (except for passing them as arguments to functions). LuaComponents instead, get a proper metatable exposing the values set in the Table property. A Common pattern is to return the LuaComponent of an Actor to Lua, instead of the actor itself:

![PlayerToLuaComponent](Screenshots/PlayerToLuaComponent.PNG?raw=true "PlayerToLuaComponent")

Note that you can eventually assign an automatic metatable to UObjects too, using the LuaState UserData MetaTable features:
https://github.com/rdeioris/LuaMachine/blob/master/Tutorials/ReflectionShell.md

## Exposed Functions

### LuaCallFunction

```cpp
UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Args"))
FLuaValue LuaCallFunction(FString FunctionName, TArray<FLuaValue> Args, bool bGlobal);
```

Call a Lua function by its name. If bGlobal is false, the name will be searched in the component-associated virtual table.
The userdata is passed as the first argument.

### LuaCallValue

```cpp
UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Args"))
FLuaValue LuaCallValue(FLuaValue Value, TArray<FLuaValue> Args);
```

Takes a callable LuaValue and execute it passing userdata as the first argument.

### LuaGetField

```cpp
UFUNCTION(BlueprintCallable, BlueprintPure)
FLuaValue LuaGetField(FString Name);
```

Get a field value from the userdata virtual table

### LuaSetField

```cpp
UFUNCTION(BlueprintCallable)
void LuaSetField(FString Name, FLuaValue Value);
```

Set a field in the userdata virtual table

## Exposed Events

### OnLuaError

Triggered when the lua virtual machine generates an error when triggered from the LuaComponent. The error string is passed as the event argument
