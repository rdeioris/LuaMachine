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

When passing a UObject to Lua, it will check if the UObject is a LuaComponent or an Actor containing a LuaComponent in the same LuaState. In such a case the related userdata will be used instead.

As an example immagine a collision system where the 'other actor' is passed as the argument to a lua function.

If the actor has no LuaComponent's attached it, the function will get a raw userdata (it cannot do anythn relevant with it). If instead it has a LuaComponent attached to it (of the same LuaState of the calling one), the userdata will be the LuaComponent one with the related table/metatable

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
