@echo off

if not exist .\build mkdir build 
pushd build 

set "build_options=-Wall -pedantic -Wno-writable-strings -O0 -I../src -D_CRT_SECURE_NO_WARNINGS"

clang -g %build_options% -o ray.exe ../src/main.c

popd