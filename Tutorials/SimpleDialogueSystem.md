# A simple Dialogue System with LuaMachine

In this tutorial we will see how to use the LuaMachine plugin to create a dialogue system (versatile enough to implement a shop/inventory too) controlled by Lua scripts.

![Intro](SimpleDialogueSystem_Data/Intro.PNG?raw=true "Intro")

The Lua scripts will be part of the final build (they will be cooked in the pak file as bytecode) so you will not be able to change them after the game is packaged (check the 'Variations' section at the end of the tutorial for alternative approaches like using normal scripts in the filesystem).

The tutorial has been tested on Unreal Engine 4.21 and LuaMachine 20181230 starting from the 'Third Person' Blueprint Template.

Blueprints will be used for the whole tutorial, at the end of the page you will find a C++ implementation too.

## Installing the LuaMachine plugin

Download the latest zip file for your specific Unreal Engine version and operating system from the releases page: (https://github.com/rdeioris/LuaMachine/releases) and unzip it into the Plugins/ directory of your project (create it if it does not exist).

Restart Unreal Engine, and create the LuaState for our dialogue system:

* add new Blueprint Class
* select 'LuaState' from 'all classes'
* name it 'DialogueLuaState'

![DialogueLuaState](SimpleDialogueSystem_Data/DialogueLuaState.PNG?raw=true "DialogueLuaState")

A LuaState represents a specific Lua virtual machine (there can be multiple isolated Lua virtual machines in the same project).

For this project we need a single LuaState.

## The Talking Character

For the 'talking characters' (actors implementing dialogues in Lua) we will use the 'Twinblast' asset from Paragon.

![Twinblast](SimpleDialogueSystem_Data/Twinblast.PNG?raw=true "Twinblast")

It is a perfect choice as it includes facial morph targets that will give more emphasis to the dialogues by visually reporting emotional states.

![ContentBrowser](SimpleDialogueSystem_Data/ContentBrowser.PNG?raw=true "ContentBrowser")

Create a new Character Blueprint and add a camera component pointing to the face of Twinblast, a Sphere Collider for detecting when the player is near enough, a Text Render for simple in-world messages and (obviously) a LuaComponent.

Finally, add a variable of type 'LuaValue' named 'TalkingTable' (i will explain it later)

![TalkingCharacter](SimpleDialogueSystem_Data/TalkingCharacter.PNG?raw=true "TalkingCharacter")

The camera will be useful if you want to point to the face of the character during a dialogue (you will control the camera switch from lua).

Remember to assign an animation to the Mesh component to avoid Twinblast to be in 'A pose' while speaking (my choice is Idle_Relaxed)

## Preparing the DialogueLuaState

To allow the TalkingCharacter to be governed by Lua, we need to setup a new Lua virtual machine exposing a bunch of Unreal functions to Lua scripts. To accomplish this, we need to add Unreal/Blueprint functions to the DialogueLuaState class and expose them in the Table property of the LuaState.

We start with a simple 'Print' function that we will expose as the lua 'print' (overriding the default one), a 'Quit' function ending the game exposed as 'quit' and a LuaError event that will be triggered automatically when a Lua call fails:

![PrintAndLuaError](SimpleDialogueSystem_Data/PrintAndLuaError.PNG?raw=true "PrintAndLuaError")

Note that 'Print' expects a LuaValue argument (named 'Message', check it at the right of the screenshot). The only allowed parameters type (included the return value) is the LuaValue one. Functions violating this rule will not be selectable in the interface.

Once defined you can expose them using the 'Table' property of the LuaState (you can access it using the 'Class Defaults' button:

![LuaStateTable1](SimpleDialogueSystem_Data/LuaStateTable1.PNG?raw=true "LuaStateTable1")

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

![CodeAsset](SimpleDialogueSystem_Data/CodeAsset.PNG?raw=true "CodeAsset")

![TalkingScript](SimpleDialogueSystem_Data/TalkingScript.PNG?raw=true "TalkingScript")

Now setup the TalkingCharacter Event Graph to load and execute the specific CodeAsset on BeginPlay event and to call the character:begin_play function (note the colon syntactic sugar to allow the Lua vm to automatically includes a 'self' variable mapped to the LuaComponent, more on this later)

![BeginPlay](SimpleDialogueSystem_Data/BeginPlay.PNG?raw=true "BeginPlay")

The 'LuaRunCodeAsset' node will load and execute a LuaCode asset and will store its return value (the 'character' local table) in the TalkingTable variable (we created it earlier).

We can use this variable for the whole lifecycle of the TalkingCharacter to call the functions exposed by the lua table mapped to it (only begin_play() for now).

Pay attention to the 'State' parameter of 'LuaRunCodeAsset' as you must set it to 'DialogueLuaState'.

Finally, the 'LuaCallTableKey' node (exposed by the LuaComponent) will check if the passed table exposes the specified 'Key' and, if it is a function, will call it.

Note that nodes exposed by the LuaComponent will automatically pass the LuaComponent itself as the first argument of called functions. As we are using the 'table:function' syntax in our script, we will get this parameter in the 'self' variable automagically. We will use the 'self' variable later in the tutorial.

Drag the TalkingCharacter blueprint to the Level and hit Play.

If all goes well you should get a 'Hello World' message on screen.

## More Unreal Engine events with Lua

Time to implement overlapping events for the sphere collider as well as a custom 'Speak' event that will be triggered by the player when wanting to talk to a specific 'TalkingCharacter':

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

Add the new events to the TalkingCharacter's Event Graph (pay attention to the overlapping events as they must be related to the sphere, not the capsule of the character):

![Overlap](SimpleDialogueSystem_Data/Overlap.PNG?raw=true "Overlap")

Note that the overlapping actor is passed as the first argument of the Lua function

'Speak' is a custom event:

![Speak](SimpleDialogueSystem_Data/Speak.PNG?raw=true "Speak")

Re-Play and try to overlap the TalkingCharacter Sphere with the Mannequin, if all goes well you should see 'Begin Overlap' printed when the player is near the TalkingCharacter and 'End Overlap' printed when the player is far from it.

## Dealing with multiple TalkingCharacters and checking which actor is overlapping

Our game could include dozens of characters or objects overlapping the collision sphere of the TalkingCharacter. For this reason
we need a way to check that the overlapping actor is the player pawn.

As getting the player pawn from Lua is a pretty handy function, very probably the best thing to do is exposing it as a global Lua function in the same way we exposed 'print':

![GetPlayer](SimpleDialogueSystem_Data/GetPlayer.PNG?raw=true "GetPlayer")

![LuaStateTable2](SimpleDialogueSystem_Data/LuaStateTable2.PNG?raw=true "LuaStateTable2")

Now we can simply do a comparison in the begin_overlap/end_overlap functions:

```lua
function character:begin_overlap(other)
  if get_player() == other then
    print('Begin Overlap')
  end
end

function character:end_overlap(other)
  if get_player() == other then
    print('End Overlap')
  end
end
```

Hit Play again and check if the overlap system still works (if you want, try placing other overlapping objects in the level to check the comparison works correctly)

As said before, multiple TalkingCharacter's could be spawned in the level, so we need a way to 'mark' which one is the currently focused character, both visually and from the Lua point of view.

Let's start with the visual part: we want to set a message (from Lua) in the TextRender component of the TalkingCharacter.

This is a function specific of the TalkingCharacter so we should expose it in its LuaComponent (we can access it using the automagic 'self' table)

Add the function to the TalkingCharacter Blueprint

![SetFlash](SimpleDialogueSystem_Data/SetFlash.PNG?raw=true "SetFlash")

And expose it to Lua (select the LuaComponent to access the properties in the Details panel)

![SetFlashExposed](SimpleDialogueSystem_Data/SetFlashExposed.PNG?raw=true "SetFlashExposed")

Update the code again:

```lua

function character:begin_play()
  -- ensure the TextRender content is empty
  self.flash('')
end

function character:begin_overlap(other)
  if get_player() == other then
    self.flash('Speak with Twinblast')
  end
end

function character:end_overlap(other)
  if get_player() == other then
    self.flash('')
  end
end
```

Play and check if the TextRender component activates itself when you are near the TalkingCharacter and disappears when you move away:

![SpeakWithTwinblast](SimpleDialogueSystem_Data/SpeakWithTwinblast.PNG?raw=true "SpeakWithTwinblast")

To set which one is the currently 'focused' TalkingCharacter we will use a global Lua variable. We can just set it in the DialogueLuaState Table (and set/unset it in the begin_overlap/end_overlap functions of the TalkingCharacter script).

![CurrentTarget](SimpleDialogueSystem_Data/CurrentTarget.PNG?raw=true "CurrentTarget")

Note that we want to store the Actor reference of the TalkingCharacter in the lua variable, but the 'self' table maps to the LuaComponent object. We can add a field to the 'self' table exposing the component's owner (the Actor) value:

![Owner](SimpleDialogueSystem_Data/Owner.PNG?raw=true "Owner")

The 'owner' field is now added to the LuaComponent's lua table (the one you get via 'self')

This is the updated script:

```lua
function character:begin_overlap(other)
  if get_player() == other then
    self.flash('Speak with Twinblast')
    current_target = self.owner
  end
end

function character:end_overlap(other)
  if get_player() == other then
    self.flash('')
    current_target = nil
  end
end
```

## Triggering the 'Speak' event

When a TalkingCharacter is highlighted/targeted we want to trigger its 'Speak' event by clicking the left mouse button. To accomplish
this we can manage the mouse click in the ThirdPersonCharacter Blueprint (would be the most logical choice), but to avoid clobbering the template blueprint we will use the Level Blueprint instead (suboptimal but it should work flawlessly):

![LevelBlueprint](SimpleDialogueSystem_Data/LevelBlueprint.PNG?raw=true "LevelBlueprint")

Play again, and if you click the left mouse button wile the TalkingCharacter is the current target, its 'speak' function will be called.

## Showing dialogues and choices

We are at the core of the tutorial: showing dialogues with questions/choices.

We need two widgets: one for the dialogue text (a simple big multiline text box) and another one for questions/choices/answers (a simple text button triggering a Lua function when clicked)

Let's start with the DialogueQuestion Widget:

![DialogueQuestionDesigner](SimpleDialogueSystem_Data/DialogueQuestionDesigner.PNG?raw=true "DialogueQuestionDesigner")

![DialogueQuestionGraph](SimpleDialogueSystem_Data/DialogueQuestionGraph.PNG?raw=true "DialogueQuestionGraph")

We have a widget with a button and a text (note: no canvas panel is required, you can remove it).

The QuestionTable variable is a LuaValue marked as 'Expose on Spawn'. To generate a question widget we need to pass it a lua table (well an array) composed of 2 items: the first one is the content of the button text, the second is a lua function triggered when the user click on the question/choice/answer (spoiler: the 'Question' name is probably not a good choice as no questions will be generated in the examples, only answers and choices :( )

Now the DialogueScreen Widget:

![DialogueScreenDesigner](SimpleDialogueSystem_Data/DialogueScreenDesigner.PNG?raw=true "DialogueScreenDesigner")

![DialogueScreenGraph](SimpleDialogueSystem_Data/DialogueScreenGraph.PNG?raw=true "DialogueScreenGraph")

This widget requires two 'Expose on Spawn' variables (again LuaValue's). The first one is the content of the Multiline TextBox (note: it is marked as readonly in the properties), the second one is a table (well an array again) with the list of questions/answers/choices from which the various DialogueQuestion Widgets will be generated.

To allow Lua opening and closing the dialogue widgets we need to expose two new functions:

![OpenDialogue](SimpleDialogueSystem_Data/OpenDialogue.PNG?raw=true "OpenDialogue")

(Note: you need to add the variable 'CurrentDialogueScreen' as a DialogueScreen Widget reference)

![CloseDialogue](SimpleDialogueSystem_Data/CloseDialogue.PNG?raw=true "CloseDialogue")

![LuaStateTable3](SimpleDialogueSystem_Data/LuaStateTable3.PNG?raw=true "LuaStateTable3")

We can now update the code to manage the dialogues when the 'Speak' event is triggered

```lua
function character:speak()

  function page1()
    open_dialogue('Go to page 2?', {
      {'Yes', page2},
      {'No', close_dialogue},
    })  
  end

  function page2()
    open_dialogue('Back to page 1?', {
      {'Yes', page1},
      {'No', close_dialogue},
      {'Go to page 3, please...', page3}
    }) 
  end

  function page3()
    open_dialogue([[
This is the last page,
What do you want to do ?
    ]], {
      {'Quit Game', quit},
      {'Close Dialogue', close_dialogue},      
    })
  end

  -- called by the 'Speak' event
  page1()
end
```

Note that the open/close dialogue logic implemented in the Blueprint will prevent multiple dialogues from opening concurrently. This will simplify scripting too as we just need to call
open_dialogue() again from the callback to change the content.

If all goes well you should end with something very similar:

![Page2](SimpleDialogueSystem_Data/Page2.PNG?raw=true "Page2")

## More Lua functions: change the current camera and Character's face

We will add two more blueprint functions to the TalkingCharacter Blueprint (remember to expose them the to the Table property of the LuaComponent)

![SetCamera](SimpleDialogueSystem_Data/SetCamera.PNG?raw=true "SetCamera")

![SetFace](SimpleDialogueSystem_Data/SetFace.PNG?raw=true "SetFace")

Now refactor the code to change the camera when starting a dialogue session and resetting it when exiting:

```lua
function character:speak()

  -- set camera to the character's one
  self.set_camera(self.owner)

  -- implement a close callback that will reset both camera and face
  function close()
    close_dialogue()
    -- we can use self here as we are in a closure
    self.set_camera(get_player())
    self.set_face(0.0)
  end

  function page1()
    open_dialogue('Go to page 2?', {
      {'Yes', page2},
      {'No', close},
    })  
  end

  function page2()
    open_dialogue('Back to page 1?', {
      {'Yes', page1},
      {'No', close},
      {'Go to page 3, please...', page3}
    }) 
  end

  function page3()
    self.set_face(1.0)
    open_dialogue([[
This is the last page,
What do you want to do ?
    ]], {
      {'Quit Game', quit},
      {'Close Dialogue', close},      
    })
  end

  -- called by the 'Speak' event
  page1()
end
```

When going to page3 the facial expression of Twinblast should change to the 'angry' one:

![Page3](SimpleDialogueSystem_Data/Page3.PNG?raw=true "Page3")

Challenge: improve the set_camera() function to allow for custom blend time specified from Lua.

Note: is it better to place set_camera() as a global function in the DialogueLuaState ?

## More Talking Characters

We can now refactor the TalkingCharacter Blueprint to allow to map a different script to each instance:

![MultipleCharacters](SimpleDialogueSystem_Data/MultipleCharacters.PNG?raw=true "MultipleCharacters")

Just create two more simple scripts (you are not required to implement all the events):

```lua
local character = {}

function character:begin_play()
  self.flash('I am another begin_play() function...')
end

return character
```

```lua
local character = {}

function character:begin_play()
  self.flash('Too far to see my name...')
  self.set_face(1.0)
end

return character
```

![ThreeScripts](SimpleDialogueSystem_Data/ThreeScripts.PNG?raw=true "ThreeScripts")

## Inventory/Shop system

We can easily implement shops/inventories:

```lua
local merchant = {}

merchant.shop_sign = string.format('Shop managed by %s', _VERSION)

merchant.items = {'Shotgun', 'Grenade', 'Guns', 'LaserGun', 'Bazooka'}

function merchant:begin_play()
  self.flash(merchant.shop_sign)
end

function merchant:begin_overlap(other)
  if get_player() == other then
    self.flash('Buy something')
    current_target = self.owner
  end
end

function merchant:end_overlap(other)
  if get_player() == other then
    self.flash(merchant.shop_sign)
    current_target = nil
  end
end

function merchant:speak()

  -- reset when closing the shop
  function close()
    self.set_camera(get_player())
    close_dialogue()
  end

  -- special closure for generating a callback with the index of the item to buy
  function buy(index)
    return function()
        table.remove(merchant.items, index)
        show_items()
    end
  end

  function show_items()
    self.set_camera(self.owner)

    -- build the list of items (and the related callbacks)
    local items = {}
    for k,v in pairs(merchant.items) do
      -- v is the item name, k is its index (we use it as in lua we can only remove efficiently by index)
      table.insert(items, {'Buy ' .. v, buy(k)})
    end
    table.insert(items, {'Nothing, Thanks', close})

    open_dialogue('Welcome to my humble shop', items)
  end

  -- triggered by 'Speak' event
  show_items()
end

return merchant
```

![Shop](SimpleDialogueSystem_Data/Shop.PNG?raw=true "Shop")

Let's improve it by adding the 'gold/money' concept:

First, add a player_gold Integer item in the Table of the DialogueLuaState and set to something like '4'. In this
way the merchant will be able to access the amount of player gold.

```lua
local merchant = {}

merchant.shop_sign = string.format('Shop managed by %s', _VERSION)

merchant.items = {'Shotgun', 'Grenade', 'Guns', 'LaserGun', 'Bazooka'}

-- increases whenever the merchant sells something
merchant.gold = 0

function merchant:begin_play()
  self.flash(merchant.shop_sign)
end

function merchant:begin_overlap(other)
  if get_player() == other then
    self.flash('Buy something')
    current_target = self.owner
  end
end

function merchant:end_overlap(other)
  if get_player() == other then
    self.flash(merchant.shop_sign)
    current_target = nil
  end
end

function merchant:speak()

  -- reset when closing the shop
  function close()
    self.set_camera(get_player())
    close_dialogue()
  end

  -- special closure for generating a callback with the index of the item to buy
  function buy(index)
    return function()
        if player_gold <= 0 then
          open_dialogue('Not enough gold', {{'Sorry', show_items}})
        else
          table.remove(merchant.items, index)
          merchant.gold = merchant.gold + 1
          player_gold = player_gold - 1
          show_items()
        end
    end
  end

  function show_items()
    self.set_camera(self.owner)

    -- build the list of items (and the related callbacks)
    local items = {}
    for k,v in pairs(merchant.items) do
      -- v is the item name, k is its index (we use it as in lua we can only remove efficiently by index)
      table.insert(items, {'Buy ' .. v, buy(k)})
    end
    table.insert(items, {'Nothing, Thanks', close})

    open_dialogue([[Welcome to my humble shop,
      You have ]] .. player_gold .. ' gold', items)
  end

  -- triggered by 'Speak' event
  show_items()
end

return merchant
```

## Cooking and Packaging

One of the advantages of using LuaCode assets, is that they will be built as normal assets and they will be converted to bytecode by default. The cooking system is smart enough to adapt the bytecode even if you want to build the project for systems (like android) having low level differences (that would break on bytecode generated on another system).

## Variations

The tutorial aims at showing a simple approach, but you can obviously using various techniques. The following variations could be of interest:

### From LuaCode Asset to filesystem-based scripts

If you want to allow the user to modify/hack your game after it has been built, you can use normal filesystem-based lua scripts.

Just place them in the Content directory (it is added by default to the Lua package search path) and change the TalkingCharacter Blueprint to use the filename as the source script:

![FromFile](SimpleDialogueSystem_Data/FromFile.PNG?raw=true "FromFile")

Eventually you can use both (to allow some form of modding):

![Modding](SimpleDialogueSystem_Data/Modding.PNG?raw=true "Modding")

We first check for a Lua file, if it is not available (returns nil in such a case), we use the LuaCode asset.

### Using C++ instead of Blueprint Nodes

Instead of exposing Blueprint functions to Lua, you can expose Cpp functions.

This is the code for a CppDialogueLuaState class that you can use in place of the DialogueLuaState one:

```cpp
#pragma once

#include "CoreMinimal.h"
#include "LuaState.h"
#include "CppDialogueLuaState.generated.h"

/**
 * 
 */
UCLASS()
class TUTORIALLUADIALOGUE_API UCppDialogueLuaState : public ULuaState
{
	GENERATED_BODY()

public:
	UCppDialogueLuaState();

	UFUNCTION()
	FLuaValue GetPlayer();

	UFUNCTION()
	void Quit();

	UFUNCTION()
	void Print(FLuaValue Message);
	
};
```

```cpp
#include "CppDialogueLuaState.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/GameEngine.h"

UCppDialogueLuaState::UCppDialogueLuaState()
{
	Table.Add("get_player", FLuaValue::Function(GET_FUNCTION_NAME_CHECKED(UCppDialogueLuaState, GetPlayer)));
	Table.Add("quit", FLuaValue::Function(GET_FUNCTION_NAME_CHECKED(UCppDialogueLuaState, Quit)));
	Table.Add("print", FLuaValue::Function(GET_FUNCTION_NAME_CHECKED(UCppDialogueLuaState, Print)));

	Table.Add("current_target", FLuaValue());
	Table.Add("player_gold", FLuaValue(4));
}

FLuaValue UCppDialogueLuaState::GetPlayer()
{
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	return PlayerController ? FLuaValue(PlayerController->GetPawn()) : FLuaValue();
}

void UCppDialogueLuaState::Quit()
{
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (PlayerController)
	{
		PlayerController->ConsoleCommand("quit");
	}
}

void UCppDialogueLuaState::Print(FLuaValue Message)
{
	GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Cyan, *Message.ToString());
}

```
