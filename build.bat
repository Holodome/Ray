@echo off

if not exist .\build mkdir build 

REM Build .lib's if not present
if not exist build\stb_image_write.lib (
    call scripts\build_stb.bat
)

robocopy data build\data /E /njh /njs /ndl /nc /ns

pushd build 

set "RayDisabledErrors=-Wall -Wno-writable-strings -Wno-unused-function -Wno-unused-variable -Wparentheses -Wswitch -Wno-c99-designator"
set "RayLinkedLibraries= -L./ -luser32 -lkernel32 -lgdi32 -lstb_image_write -lstb_sprintf -lstb_image -lstb_truetype"
set "RayCompilerSwitches=-D_CRT_SECURE_NO_WARNINGS"
set "RayBuildOptions= %RayCompilerSwitches% -gcodeview -O3 -I../ray -I../thirdparty %RayDisabledErrors% %RayLinkedLibraries% -x c -std=c11 -fno-exceptions -fno-math-errno"

clang -g %RayBuildOptions% -o ray.exe ../ray/ray/ray.c 

set "EditorDisabledErrors=-Wall -Wno-writable-strings -Wno-unused-function -Wparentheses -Wswitch -Wno-c99-designator"
set "EditorLinkedLibraries= -L./ -luser32 -lkernel32 -lgdi32 -lstb_sprintf"
set "EditorCompilerSwitches=-D_CRT_SECURE_NO_WARNINGS"
set "EditorBuildOptions= %EditorCompilerSwitches% -gcodeview -O3 -I../ray -I../editor -I../thirdparty %EditorDisabledErrors% %EditorLinkedLibraries% -std=c++11 -fno-exceptions -fno-math-errno -Xlinker /subsystem:console"

REM clang -g %EditorBuildOptions% -o editor.exe ../editor/editor.cc 

popd 

if %ERRORLEVEL% neq 0 goto :end

if not exist .\data mkdir data 

build\ray.exe -out data\out.png -open -rpp 128

:end