@echo off
REM Builds stb libraries as .lib for windows

REM Needed for lib.exe
where cl /q
if %ERRORLEVEL% neq 0 call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

if not exist .\build mkdir build 
pushd build 

copy ..\src\Thirdparty\stb_image_write.h stb_image_write.c

clang-cl -DSTB_IMAGE_WRITE_IMPLEMENTATION=1 -c -O2 -EHsc -nologo stb_image_write.c
lib -nologo -out:stb_image_write.lib stb_image_write.obj

del stb_image_write.c

popd 