# A simple Dialogue System with LuaMachine

In this tutorial we will see how to use the LuaMachine plugin to create a dialogue system (versatile enough to implement a shop/inventory too) controlled by Lua scripts.

The Lua scripts will be part of the final build (they will be cooked in the pak file as bytecode) so you will not be able to change them after the game is packaged (check the 'Variations' section at the end of the tutorial for alternative approaches like using normal scripts in the filesystem).

The tutorial has beed tested on Unreal Engine 4.21 and LuaMachine 20181229 starting from the 'Third Person' Blueprint Template.

## Installing the LuaMachine plugin

Download the latest zip file for your specific Unreal Engine version and operating system from the releases page: (https://github.com/rdeioris/LuaMachine/releases) and unzip it into the Plugins/ directory of your project (create it if it does not exist).

Restart Unreal Engine, and create the LuaState for our dialogue system:

* add new blueprint class
* select 'LuaState' from 'all classes'
* name it 'DialogueLuaState'

A LuaState represents a specific Lua virtual machine (there can be multiple isolated Lua virtual machines in the same project).

For this project we need a single LuaState.

## The Talking Character

For the 'talking characters' (actors implementing dialogues in Lua) we will use the 'Twinblast' asset from Paragon. It is a perfect choice as it includes facial morph targets that will give more emphasis to the dialogues by visually reporting emotional states.

Create a new Character Blueprint and add a camera component pointing to the face of Twinblast, A Sphere Collider for detecting when the player is near enough, a Text Render for simple in-world messages and (obviously) a LuaComponent:

The camera will be useful if you want to point to the face of the character during a dialogue (you will control the camera switch from lua).

## Preparing the DialogueLuaState

To allow the TalkingCharacter to be governed by Lua, we need to setup a new Lua virtual machine exposing a bunch of Unreal functions to Lua scripts. To accomplish this, we need to add Unreal/Blueprint functions to the DialogueLuaState class and expose them in the Table property of the LuaState.

We start with a simple 'Print' function that we will expose as the lua 'print' (overriding the default one) and a LuaError event that will be triggered automatically when a Lua call fails:



## The first script

Create a new LuaCode asset in your Content Browser (it is under the 'Miscellaneous' group) and insert the following code:

```lua
local character = {}

function character:begin_play()
  print('Hello World')
end

return character
```

Now build the TalkingCharacter Graph to load and execute the CodeAsset on BeginPlay event and to call the character:begin_play function (note the colon syntactic sugar to allow the Lua vm to automatically includes a 'self' variable mapped to the LuaComponent, more on this later)



## More Unreal Engine events with Lua

Time to implement overlaping events.

Add a new collision sphere that will trigger lua whenever something overlaps it

```lua
local character = {}

function character:begin_play()
  print('Hello World')
end

function character:begin_overlap(other)
  print('Begin Overlap')
end

function character:end_overlap(other)
  print('End Overlap')
end

return character
```


```lua
local character = {}

function character:begin_play()
  print('Hello World')
end

function character:begin_overlap(other)
  if is_player_pawn(other) then
    print('Begin Overlap')
  end
end

function character:end_overlap(other)
  if is_player_pawn(other) then
    print('End Overlap')
  end
end

return character
```


## Showing dialogues and choices

## The Lua api

## Cooking and Packaging

## From LuaCode Asset to filesystem-based scripts

## Variations
