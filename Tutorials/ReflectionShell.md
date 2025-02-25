# Implementing a Lua Reflection Shell (WORK IN PROGRESS)

The main objective of the LuaMachine project is to safely expose Lua apis to scripters (and modders), so the Unreal Engine reflection system is
completely hidden by default.

If you know what you are doing, you can implement UObject's reflection easily in LuaMachine using C++.

This tutorial will show you how (with a bunch of C++ lines) you can inspect/modify UObject's at runtime (and obviously make mess everywhere).

Note: this has been tested with LuaMachine 20190122 and Unreal Engine 4.21 using the ThirdPersonCharacter template.

## Creating the LuaReflectionState

As always, we start by creating a new ULuaState. This will be the base (we will work on an inherited Blueprint class) for the Lua VM targeted at reflection.

Just create a new C++ class named 'LuaReflectionStateBase' inheriting from 'LuaState', and create a blueprint subclass of it named 'LuaReflectionState'

## Assigning a 'Lua name' to an Actor

Next step is creating a component that will automatically assign an actor to a Lua global name:

* Create a new Blueprint Class of type ActorComponent and name it 'LuaReflectionComponent'
* Add a variable of type 'class of LuaReflectionStateBase' named 'State', and assign it a default value of 'LuaReflectionState'
* Add a public variable of type 'String' (this will be the name exposed to the Lua VM)
* Finally assign the 'GetOwner()' return value to the Lua name (in the 'Begin Play' event)

Note the usage of 'Lua Create Object in State' that will enforce this LuaValue to be mapped to a specific state (this will avoid mess in case of various Lua VM running in the same process)

## Adding the Runtime Shell

Time to add the 'overlay shell' to our screen. From this Multiline Editable Text we will issue lua commands

## The core of our reflection system: adding a MetaTable to a UObject

```cpp
LuaReflectionStateBase::ULuaReflectionStateBase()
{
	Table.Add("unreal_get_name", FLuaValue::Function(GET_FUNCTION_NAME_CHECKED(ULuaReflectionStateBase, UnrealGetName)));
}
```

```cpp
FLuaValue ULuaReflectionStateBase::UnrealGetName(FLuaValue Value)
{
	if (!Value.Object)
		return FLuaValue();

	return FLuaValue(Value.Object->GetName());
}
```

```lua
local reflection = {}

function reflection:__tostring()
  return string.format('uobject: %s', unreal_get_name(self))
end

function reflection:__index(key)
  return unreal_get_property(self, key)
end

function reflection:__newindex(key, value)
  return unreal_set_property(self, key, value)
end

return reflection
```

## Getting the list of UProperties

```cpp
FLuaValue ULuaReflectionStateBase::UnrealGetProperties(FLuaValue Value)
{
	if (!Value.Object)
		return FLuaValue();

	UStruct* Class = Cast<UStruct>(Value.Object);
	if (!Class)
		Class = Value.Object->GetClass();

	FLuaValue PropertiesArray = CreateLuaTable();
	// remember that Lua starts counting from 1
	int32 Index = 1;

	for (TFieldIterator<UProperty> PropsIterator(Class); PropsIterator; ++PropsIterator)
	{
		UProperty* Property = *PropsIterator;
		PropertiesArray.SetFieldByIndex(Index++, FLuaValue(Property->GetName()));
	}

	return PropertiesArray;
}
```

Please py attention to the weird naming scheme as i am casting to UStruct but i am calling it 'Class'. This will be able to support USTRUCT's too if you want to improve the system.

Now let's test it by calling Lua's unreal_get_properties() from the Mannequin's graph:



## Getting/Setting Properties

```cpp
FLuaValue ULuaReflectionStateBase::UnrealGetProperty(FLuaValue Value, FLuaValue Name, FLuaValue Index)
{
	if (!Value.Object)
		return FLuaValue();
	
	UStruct* Class = Cast<UStruct>(Value.Object);
	if (!Class)
		Class = Value.Object->GetClass();

	UProperty* Property = Class->FindPropertyByName(Name.ToName());
	if (Property)
	{
		bool bSuccess;
		return FromUProperty(Value.Object, Property, bSuccess, Index.ToInteger());
	}

	return FLuaValue();
}

void ULuaReflectionStateBase::UnrealSetProperty(FLuaValue Value, FLuaValue Name, FLuaValue NewValue, FLuaValue Index)
{
	if (!Value.Object)
		return;

	UStruct* Class = Cast<UStruct>(Value.Object);
	if (!Class)
		Class = Value.Object->GetClass();

	UProperty* Property = Class->FindPropertyByName(Name.ToName());
	if (Property)
	{
		bool bSuccess;
		ToUProperty(Value.Object, Property, NewValue, bSuccess, Index.ToInteger());
	}
}
```


## Managing ArrayProperties

## Bonus: calling UObject's methods

```cpp
FLuaValue ULuaReflectionState::CallUObjectMethod(FLuaValue Object, FLuaValue Method, FLuaValue Args)
{
	if (Object.Type != ELuaValueType::UObject)
		return FLuaValue();

	if (Method.Type != ELuaValueType::UObject)
		return FLuaValue();

	UFunction* Function = (UFunction *)Method.Object;

	FScopeCycleCounterUObject ObjectScope(Object.Object);
	FScopeCycleCounterUObject FunctionScope(Function);

	void* Parameters = FMemory_Alloca(Function->ParmsSize);
	FMemory::Memzero(Parameters, Function->ParmsSize);

	// arguments
	if (Args.Type == ELuaValueType::Table)
	{
		int32 NArgs = 1;
		for (TFieldIterator<UProperty> FArgs(Function); FArgs && ((FArgs->PropertyFlags & (CPF_Parm | CPF_ReturnParm)) == CPF_Parm); ++FArgs)
		{
			UProperty *Prop = *FArgs;

			FLuaValue Parameter = Args.GetFieldByIndex(NArgs++);
			bool bSuccess;
			ToUProperty((UObject *)Parameters, Prop, Parameter, bSuccess);
		}
	}

	Object.Object->ProcessEvent(Function, Parameters);

	// get return value
	for (TFieldIterator<UProperty> FArgs(Function); FArgs; ++FArgs)
	{
		UProperty *Prop = *FArgs;
		if (!Prop->HasAnyPropertyFlags(CPF_ReturnParm | CPF_OutParm))
			continue;
		bool bSuccess;
		return FromUProperty((UObject *)Parameters, Prop, bSuccess);
	}

	return FLuaValue();
}
```

## Bonus: calling static methods from Blueprint Function Libraries

## Bonus: binding Lua functions to UObject's events

```cpp
void ULuaReflectionState::BindEvent(FLuaValue Object, FLuaValue Name, FLuaValue Callable)
{
	if (Object.Type != ELuaValueType::UObject)
		return;

	if (Name.Type != ELuaValueType::String)
		return;

	UStruct* Class = Cast<UStruct>(Object.Object);
	if (!Class)
		Class = Object.Object->GetClass();

	UProperty* Property = Class->FindPropertyByName(Name.ToName());
	if (Property)
	{
		UMulticastDelegateProperty* MulticastDelegateProperty = Cast<UMulticastDelegateProperty>(Property);
		if (MulticastDelegateProperty)
		{
			FMulticastScriptDelegate* MulticastDelegate = MulticastDelegateProperty->GetPropertyValuePtr_InContainer(Object.Object);
			ULuaDelegate *LuaDelegate = NewObject<ULuaDelegate>();
			LuaDelegate->SetLuaCallableAndSignature(Callable, MulticastDelegateProperty->SignatureFunction);
			FScriptDelegate Delegate;
			Delegate.BindUFunction(LuaDelegate, "LuaFakeUFunction");
			MulticastDelegate->Add(Delegate);
		}
	}
}

void ULuaDelegate::ProcessEvent(UFunction *Function, void *Parms)
{
	if (!Callable.LuaState)
		return;

	// arguments
	TArray<FLuaValue> Args;
	for (TFieldIterator<UProperty> FArgs(CallableSignature); FArgs && ((FArgs->PropertyFlags & (CPF_Parm | CPF_ReturnParm)) == CPF_Parm); ++FArgs)
	{
		UProperty *Prop = *FArgs;
		bool bSuccess;
		Args.Add(Callable.LuaState->FromUProperty((UObject *)Parms, Prop, bSuccess));
	}

	ULuaBlueprintFunctionLibrary::LuaValueCall(Callable, Args);
}

void ULuaDelegate::LuaFakeUFunction()
{

}
```

## Bonus: working with USTRUCTs

## Super Bonus: running lua scripts from the Unreal Editor


