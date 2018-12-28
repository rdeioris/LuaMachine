# A simple Dialogue System with LuaMachine

In this tutorial we will see how to use the LuaMachine plugin to implement a dialogue system controlled by Lua scripts.

The Lua scripts will be part of the final build (they will be cooked in the pak file as bytecode) so you will not be able to change them after the game is packaged (check the 'Variations' section at the end of the tutorial for using normal scripts in the filesystem).

The tutorial has beed tested on Unreal Engine 4.21 and LuaMachine 20181228 starting from the Third Person Blueprint template.

## Installing the LuaMachine plugin

Download the latest zip file for your specific unreal engine version and operating system from the releases page: https://github.com/rdeioris/LuaMachine/releases and unzip it into the Plugins directory of your project (create it, if it does not exist).

Restart Unreal Engine, and create the LuaState for our dialogue system:

* add new blueprint class
* select 'LuaState' from 'all classes'
* name it 'DialogueLuaState'

A LuaState represents a specific Lua virtual machine (there can be multiple isolated lua virtual machines in the same project).

For this project we need a single LuaState (in the 'Variations' section you will find a different approach using a LuaState for each character)

## The Talking Character

For the 'talking characters' (actors implementing dialogues in Lua) we will use the 'Twinblast' asset from Paragon. It is a perfect choice as it includes facial morph targets that will give more emphasis to the dialogues by visually reporting his emotional state.

Create a new Character Blueprint and add a camera component pointing to the face of Twinblast and a LuaComponent:

The camera will be useful if you want to point to the face of the character during a dialogue (you will control the camera switch from lua)

## Preparing the DialogueLuaState

To allow the TalkingCharacter to be governed by Lua, we need to setup a new Lua virtual machine, exposing a bunch of Unreal functions to scripts. To accomplish this, we need to add Unreal/Blueprint functions to the DialogueLuaState class and expose them in the Table property of the LuaState.

We start with a simple printing function that we will expose as the lua 'print' and a LuaError event, that will be triggered automatically when a Lua call fails:





## The first script

Create a new LuaCode asset in your Content Browser (it is under the 'Miscellaneous' group) and insert the following code:

```lua
local character = {}

function character:begin_play()
  print('Hello World')
end

return character
```

Now instructs the TalkingCharacter to load the CodeAsset on BeginPlay event and to call the character:begin_play function (note the colon syntactic sugar to allow the Lua vm to automatically includes a 'self' variable pointing the LuaComponent, more on this later)



## More Unreal Engine events with Lua

Time to implement overlaping events.

Add a new collision sphere that will trigger lua whenever something overlaps it

```lua
local character = {}

function character:begin_play()
  print('Hello World')
end

function character:begin_overlap()
  print('Begin Overlap')
end

function character:end_overlap()
  print('End Overlap')
end

return character
```





## Showing dialogues and choices

## The Lua api

## Cooking and Packaging

## From LuaCode Asset to filesystem-based scripts

## Variations
