# Using Lua Coroutines

Coroutines in lua are "interruptable" functions: they can 'suspend' themselves and can be 'resumed' by another coroutine or from the main code:

```lua
coro = coroutine.create(function()
  -- suspend function (without returning values)
  coroutine.yield()
  -- suspend returning 5 values
  coroutine.yield('one', 'two', 'three', 'four', 'five')
  -- suspend returning 1 value
  coroutine.yield(17)
  -- suspend returning 1 value
  coroutine.yield('ended')
end)

-- run the coroutine until the first yield
alive = coroutine.resume(coro)

-- ... until second yield (it returns five values)
alive, one, two, three, four, five = coroutine.resume(coro)

-- ... until third yield
alive, seventeen = coroutine.resume(coro)

-- ... the last yield
alive, ended = coroutine.resume(coro)

-- complete the coroutine
alive = coroutine.resume(coro)

-- this time 'alive' will be 'false' as the coroutine is 'dead' (it has finished its work)
alive = coroutine.resume(coro)
```

Note that albeit coroutine.yield() can be called without arguments, the coroutine.resume() function will always get the first return value as a boolean specifying if the coroutine is still alive or not.

More infos can be found here:

https://www.lua.org/manual/5.3/manual.html#2.6

Coroutines maps to native lua objects named 'threads'. Lua Threads are fully supported by the FLuaValue USTRUCT, so you can create a coroutine from lua and pass it to Unreal Engine functions:


![Coroutine](Screenshots/Coroutine.PNG?raw=true "Coroutine")

The BeginPlay event will create a new coroutine (a thread LuaValue) and stores it in a blueprint variable.

At each tick the coroutine.resume() lua function is called passing it the coroutine as the first argument. Note the usage of the LuaGlobalCallMulti node as it allows to receive multiple return values. Remember that the first return value of coroutine.resume() is always the 'status' of the coroutine (true is alive, false is dead)
