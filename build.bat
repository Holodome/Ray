@echo off

if not exist .\build mkdir build 

if not exist build\stb_image_write.lib (
    call scripts\build_stb.bat
)

pushd build 

set "DisabledErrors=-Wall -Wno-writable-strings -Wno-unused-function -Wparentheses -Wswitch"
set "LinkedLibraries= -L./ -luser32 -lkernel32 -lgdi32 -lstb_image_write -lstb_sprintf -lstb_image -lstb_truetype"
set "CompilerSwitches=-D_CRT_SECURE_NO_WARNINGS"
set "BuildOptions= %CompilerSwitches% -gcodeview -O0 -mfpmath=sse -I../src %DisabledErrors% %LinkedLibraries% -x c -std=c11 -fno-exceptions -fno-math-errno -Wno-c99-designator"

REM Build .lib's if not present


clang -g %BuildOptions% -o ray.exe ../src/ray/ray.c 
REM clang -g %BuildOptions% -o editor.exe ../src/editor/editor.c 

popd 


robocopy data build\data /E /njh /njs /ndl /nc /ns


if %ERRORLEVEL% neq 0 goto :end



if not exist .\data mkdir data 
pushd data 

REM ..\build\ray.exe
REM start out.png

popd 

:end