# Tips & Tricks

In this page users can publish their c++ snippets (please make pull requests with your credits)

## Running lua files recursively from a directory and get their return value

Author: Roberto De Ioris

C++/UFUNCTION code:

```cpp
#include "HAL/FileManager.h"

ULuaStateOrLuaPackage::ULuaStateOrLuaPackage()
{
	Table.Add("scan_and_run", FLuaValue::Function(GET_FUNCTION_NAME_CHECKED(ULuaStateOrLuaPackage, ScanAndRunLuaScripts)));
}

TArray<FLuaValue> ULuaStateOrLuaPackage::ScanAndRunLuaScripts(FLuaValue Directory)
{
	TArray<FLuaValue> Results;
	TArray<FString> Filenames;
	IFileManager::Get().FindFilesRecursive(Filenames, *Directory.ToString(), TEXT("*.lua"), true, false);

	for (FString Filename : Filenames)
	{
		Results.Add(ULuaBlueprintFunctionLibrary::LuaRunNonContentFile(GetWorld(), GetLuaState(), Filename, false));
	}

	return Results;
}
```

Lua script:

```lua
items = {foobar.scan_and_run('D:/luascripts')}

for _, ret in ipairs(items) do
  print(ret)
end
```

## Wrapping Vectors in Lua tables with math support via metatables

The Goal:

```lua
a = vector:new(1,2,3)
b = vector:new(4,5,6)

print(a)
print(b)

print(a + b)
```

The Implementation:

(ULuaVector is a subclass of ULuaBlueprintPackage and must be added to the LuaState Blueprint Package Table as 'vector')

```cpp
#include "LuaVector.h"
#include "LuaBlueprintFunctionLibrary.h"

ULuaVector::ULuaVector()
{
	Table.Add("new", FLuaValue::Function(GET_FUNCTION_NAME_CHECKED(ULuaVector, VectorNew)));
	Table.Add("__add", FLuaValue::Function(GET_FUNCTION_NAME_CHECKED(ULuaVector, VectorAdd)));
	Table.Add("__tostring", FLuaValue::Function(GET_FUNCTION_NAME_CHECKED(ULuaVector, VectorStr)));
}

FLuaValue ULuaVector::VectorNew(FLuaValue SelfPackage, FLuaValue X, FLuaValue Y, FLuaValue Z)
{
	return BuildVector(FVector(X.ToFloat(), Y.ToFloat(), Z.ToFloat()));
}

FLuaValue ULuaVector::VectorAdd(FLuaValue A, FLuaValue B)
{
	return BuildVector(ULuaBlueprintFunctionLibrary::LuaTableToVector(A) + ULuaBlueprintFunctionLibrary::LuaTableToVector(B));
}

FLuaValue ULuaVector::VectorStr(FLuaValue LuaVector)
{
	return ULuaBlueprintFunctionLibrary::LuaTableToVector(LuaVector).ToString();
}

FLuaValue ULuaVector::BuildVector(FVector Vector)
{
	FLuaValue NewTable = ULuaBlueprintFunctionLibrary::LuaCreateTable(GetWorld(), GetLuaState());
	NewTable.SetField("x", Vector.X);
	NewTable.SetField("y", Vector.Y);
	NewTable.SetField("z", Vector.Z);

	NewTable.SetMetaTable(GetSelfLuaTable());

	return NewTable;
}
```

## Mapping BlueprintFunctionLibrary to UserData

The Goal:

```lua
kismet_math = bpfl.require('KismetMathLibrary')

print(kismet_math.Abs(-17.3))

print(kismet_math.Add_DoubleDouble(17, 0.3))

print(kismet_math.Add_VectorFloat({1,2,3}, 0.3).X)
```

'bpfl' (shortcut for 'Blueprint Function Library') will be the package to get a userdata from a BlueprintFunctionLibrary ('KismetMathLibrary' on the example, but can be anything)

```cpp
UCLASS()
class UBPFLLuaBlueprintPackage : public ULuaBlueprintPackage
{
	GENERATED_BODY()
public:
	UBPFLLuaBlueprintPackage();

	UFUNCTION()
	FLuaValue RequireBlueprintFunctionLibrary(FLuaValue LibraryName);
};
```

```cpp
UBPFLLuaBlueprintPackage::UBPFLLuaBlueprintPackage()
{
	Table.Add("require", FLuaValue::Function(GET_FUNCTION_NAME_CHECKED(UBPFLLuaBlueprintPackage, RequireBlueprintFunctionLibrary)));
}

FLuaValue UBPFLLuaBlueprintPackage::RequireBlueprintFunctionLibrary(FLuaValue LibraryName)
{
	UClass* FoundClass = Cast<UClass>(StaticFindObject(UClass::StaticClass(), ANY_PACKAGE, *LibraryName.ToString()));
	if (FoundClass)
	{
		ULuaUserDataBPFL* UserDataBPFL = NewObject<ULuaUserDataBPFL>(GetLuaStateInstance());
		UserDataBPFL->InitializeWithClass(FoundClass);
		return FLuaValue(UserDataBPFL);
	}
	return FLuaValue();
}
```

The RequireBlueprintFunctionLibrary lua exposed UFunction will search for the specified library name using the reflection system, and will create a ULuaUserDataBPFL (check below) that will keep a reference to the blueprint library

```cpp
UCLASS()
class ULuaUserDataBPFL : public ULuaUserDataObject
{
	GENERATED_BODY()
	
public:
	void InitializeWithClass(UClass* InClass);

	virtual FLuaValue ReceiveLuaMetaIndex_Implementation(FLuaValue Key) override;

protected:
	UPROPERTY()
	UClass* BPFLClass;
};
```

```cpp
void ULuaUserDataBPFL::InitializeWithClass(UClass* InClass)
{
	BPFLClass = InClass;
}

FLuaValue ULuaUserDataBPFL::ReceiveLuaMetaIndex_Implementation(FLuaValue Key)
{
	UFunction* Function = BPFLClass->FindFunctionByName(*Key.ToString());
	if (Function)
	{
		return FLuaValue::FunctionOfObject(BPFLClass->GetDefaultObject(), Function->GetFName());
	}

	return FLuaValue();
}
```

The key here is the ReceiveLuaMetaIndex_Implementation override, that will return the UFunction ptr of the supplied function name (if it exists).

You can now map the UBPFLLuaBlueprintPackage to the bpfl package in your LuaState configuration and (this is required) you need to enable the bRawLuaFunctionCall too: this will allow the state to automatically convert UFunction arguments to lua values.
