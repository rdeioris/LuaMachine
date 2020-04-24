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
