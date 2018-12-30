# Building Lua static library for Mac

```sh
# edit src/Makefile and set MYCFLAGS to -mmacosx-version-min=10.12
make macosx
```

# Building Lua static library for Win64

```sh
# open visual studio shell for x64
cl /MD /O2 /c /DLUA_BUILD_AS_DLL /DLUA_COMPAT_5_2 *.c
rm lua.obj
rm luac.obj
lib /OUT:lua53_win64.lib *.obj
```

# Building Lua static library for Android

```sh
# assume ndk is installed in home with api 24
~/ndk/bin/clang -O2 -Wall -Werror -Wextra -DLUA_USE_POSIX -DLUA_USEDLOPEN -DLUA_COMPAT_5_2 -std=gnu99 -c *.c
rm lua.o
rm luac.o
 ~/ndk/bin/arm-linux-androideabi-ar.exe rcu lua53_android.a *.o
```
