# Tips & Tricks

In this page users can publish their c++ snippets (please make pull requests with your credits)

## Running lua files recursively from a directory and get their return value

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

```lua
items = {foobar.scan_and_run('D:/luascripts')}

for _, ret in ipairs(items) do
  print(ret)
end
```
