# Building Lua static library for Mac

```sh
# edit src/Makefile and set MYCFLAGS to -mmacosx-version-min=10.12 -arch arm64 -arch x86_64
make macosx
```

# Building Lua static library for Linux x86_64

```sh
# after having added -fPIC to MYCFLAGS in src/Makefile
make linux
```

# Building Lua static library for Win64

```sh
# open visual studio shell for x64 and move to the src/ directory (change cl.exe and lib.exe with their arm64 version for hololens)
cl /MD /O2 /c /DLUA_BUILD_AS_DLL /DLUA_COMPAT_5_2 *.c
del lua.obj
del luac.obj
lib /OUT:liblua53_win64.lib *.obj
```

# Building Lua static library for Android

```sh
# assume ndk is installed in home with api 24
~/ndk/bin/clang -O2 -Wall -Werror -Wextra -DLUA_USE_POSIX -DLUA_USEDLOPEN -DLUA_COMPAT_5_2 -std=gnu99 -c *.c
rm lua.o
rm luac.o
 ~/ndk/bin/arm-linux-androideabi-ar.exe rcu liblua53_android.a *.o
```

# Building Lua static library for iOS

Comment the os_execute function in loslib.c and its mapping in the syslib array

```sh
clang -arch arm64 -mios-version-min=7.0 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk/ -O2 -Wall -Werror -Wextra -DLUA_USE_POSIX -DLUA_USEDLOPEN -DLUA_COMPAT_5_2 -std=gnu99 -c *.c
rm lua.o luac.o
 ar rcu liblua53_ios.a *.o
```


# Building Lua static library for Linux AArch64

```sh
# assume cross compiler for aarch64 is available
aarch64-linux-gnu-gcc -O2 -Wall -Werror -Wextra -DLUA_USE_POSIX -DLUA_USEDLOPEN -DLUA_COMPAT_5_2 -std=gnu99 -c *.c
rm lua.o
rm luac.o
aarch64-linux-gnu-ar rcD liblua53_linux_aarch64.a *.o
```
