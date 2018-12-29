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

![DialogueLuaState](Tutorials/SimpleDialogueSystem_Data/DialogueLuaState.PNG?raw=true "DialogueLuaState")

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

You should get something very similar: (note syntax highlight ;)

Now build the TalkingCharacter Graph to load and execute the CodeAsset on BeginPlay event and to call the character:begin_play function (note the colon syntactic sugar to allow the Lua vm to automatically includes a 'self' variable mapped to the LuaComponent, more on this later)

Drag the TalkingCharacter blueprint to the Level and hit Play.

If all goes well you should get a 'Hello World' message on screen.

## More Unreal Engine events with Lua

Time to implement overlaping events for the sphere collider as well as a custom 'Speak' event that will be triggered by the player when wanting to talk to a specific 'TalkingCharacter'

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

function character:speak()
  print('Speaking...')
end

return character
```

Re-Play and try to overlap the TalkingCharacter Sphere with the Mannequin, if all goes well you should see 'Begin Overlap' printed when the player is near the TalkingCharacter and 'End Overlap' printed when the player is far from it.

## Dealing with multiple TalkingCharacters and checking which actor is overlapping

Our game could include dozens of characters or objects overlapping the collision sphere of the TalkingCharacter. For this reason
we need a way to check that the overlapping actor is the player pawn.

As getting the player pawn from Lua is a pretty handy function, very probably the best thing to do is exposing it as a global Lua function in the same way we exposed 'print':

Now we can simply do a comparison in the begin_overlap/end_overlap functions:

Hit Play again and check if the overlap system still works (if you want, try placing other overlapping objects in the level to check the comparison works correctly)

As said before, multiple TalkingCharacter's could be spawned in the level, so we need a way to 'mark' which one is the currently focused character, both visually and from the Lua point of view.

Let's start with the visual part: we want to set a message (from Lua) in the TextRender component of the TalkingCharacter.

This is a function specific of the TalkingCharacter so we should expose it in its LuaComponent (we can access it using the automagic 'self' table)

To set which one is the current 'focused' TalkingCharacter we will use a global Lua variable. We can just set it in the DialogueLuaState Table (and set/unset it in the begin_overlap/end_overlap functions of the TalkingCharacter script).

Note that we want to store the Actor reference of the TalkingCharacter in the lua variable, but the 'self' table maps to the LuaComponent object. We can add a field to the 'self' table exposing the component's owner (the Actor) value:

This is the updated script:

## Triggering the 'Speak' event

## Showing dialogues and choices

We are at the core of the tutorial: showing dialogues with questions/choices.

We need two widgets: one for the dialogue text (a simple big multiline text box) and another one for questions/choices (a simple text button triggering a Lua function when clicked)


To allow Lua opening and closing the dialogue widgets we need to expose two new functions:

We can now update the code to manage the dialogues when the 'Speak' event is triggered

Note that the system will prevent multiple dialogues from opening concurrently. This will simplify scripting too as we just need to call
open_dialogue() again from the callback to change the content.

## More Lua functions: change the current camera and Character's face

Challenge: improve the set_camera() function to allow for custom blend time specified from Lua.

## Inventory/Shop system

We can easily implement a shop for each character by simply generating the questions from a Lua table:

Le'ts improve it by adding the 'gold/money' concept:

## More Talking Characters

## Cooking and Packaging

## Variations

### From LuaCode Asset to filesystem-based scripts

