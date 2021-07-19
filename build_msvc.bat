@echo off

where cl /q
if %ERRORLEVEL% neq 0 call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

if not exist .\build mkdir build 
pushd build 

set "build_options=-nologo -DEBUG -fp:fast -O2 -Oi -Zi -FC -I"..\src" -std:c11 -D_CRT_SECURE_NO_WARNINGS"

cl %build_options% ../src/ray.c 

popd