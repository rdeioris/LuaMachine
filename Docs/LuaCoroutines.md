# Using Lua Coroutines

Corutines in lua are "interruptable" functions: they can 'suspend' themselves and can be 'resumed' by another corutines or the main code:

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
```
