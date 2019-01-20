# Implementing a Lua Reflection Shell

The main objective of the LuaMachine project is to safely expose Lua apis to scripters (and modders), so the Unreal Engine reflection system is
completely hidden by default.

If you know what you are doing, you can implement UObject's reflection easily in LuaMachine using C++.

This tutorial will show you how with a bunch of C++ lines you can inspect/modify UObject's at runtime (and obviously make mess eveywhere).

Note that is has been tested with LuaMachine 20190122 and Unreal Engine 4.21 using the ThirdPersonCharacter template.

## Creating the LuaReflectionState

## Getting the list of UProperties

## Getting/Setting Properties

## Syntactic Sugar

## Managing ArrayProperties

## The LuaReflectionComponent

## Bonus: calling UObject's methods

## Bonus: binding lua functions to UObject's events

## Bonus: converting USTRUCTs to FLuaValues

## What to do now ?

* try implementing a TMap wrapper

* Add more USTRUCTs conversions
