@echo off

if not exist .\build mkdir build 
pushd build 

set "DisabledErrors=-Wall -Wno-writable-strings -Wno-unused-function -Wparentheses -Wswitch -Wshadow"
set "LinkedLibraries= -L./ -luser32 -lkernel32 -lgdi32 -lstb_image_write -lstb_sprintf -lstb_image"
set "CompilerSwitches="
set "BuildOptions= %CompilerSwitches% -gcodeview -O0 -mfpmath=sse -I../src %DisabledErrors% %LinkedLibraries% -x c -std=c11 -fno-exceptions -fno-math-errno -Wno-c99-designator"

REM Build .lib's if not present
if not exist .\stb_image_write.lib (
    pushd ..
    call scripts\build_stb.bat
    popd 
)

rem clang -g %BuildOptions% -o ray.exe ../src/ray.c 
clang -g %BuildOptions% -o editor.exe ../src/editor/editor.c 
popd 

if %ERRORLEVEL% neq 0 goto :end



if not exist .\data mkdir data 
pushd data 

REM ..\build\ray.exe
REM start out.png

popd 

:end