@echo off

if not exist .\build mkdir build 
pushd build 

set "DisabledErrors=-Wall -Wno-writable-strings -Wno-unused-function -Wparentheses -Wswitch"
set "LinkedLibraries= -L./ -luser32 -lkernel32 -lgdi32 -lstb_image_write"
set "CompilerSwitches="
set "BuildOptions= %CompilerSwitches% -gcodeview -O0 -I../src %DisabledErrors% %LinkedLibraries% -x c -std=c11 -fno-exceptions -fno-math-errno -Wno-c99-designator"

REM Build .lib's if not present
if not exist .\stb_image_write.lib (
    pushd ..
    scripts\build_stb.bat
    popd 
)

clang -g %BuildOptions% -o ray.exe ../src/Ray.c 

popd 