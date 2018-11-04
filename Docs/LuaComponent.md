# LuaComponent

This is an ActorComponent allowing you to expose an Actor to the Lua Virtual Machine.

The Actor itself will be mapped to a lua userdata on a specific LuaState with its virtual table (and metatable)

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

Note the colon-syntax for implementing OOP style programming. Basically the LuaComponent will automatically pass the Actor as the first argument of actor.get_hp:

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
