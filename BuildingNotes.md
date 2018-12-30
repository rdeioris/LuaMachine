# Building for Mac

```sh
# edit src/Makefile and set MYCFLAGS to -mmacosx-version-min=10.12
make macosx
```

# Building for Win64

```sh
# open visual studio shell for x64
cl /MD /O2 /c /DLUA_BUILD_AS_DLL *.c
rm lua.obj
rm luac.obj
lib /OUT:lua53_win64.lib *.obj
```
