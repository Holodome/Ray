@echo off
REM Builds stb libraries as .lib for windows

REM Needed for lib.exe
where cl /q
if %ERRORLEVEL% neq 0 call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

if not exist .\build mkdir build 
pushd build 

copy ..\thirdparty\stb_image_write.h stb_image_write.c
copy ..\thirdparty\stb_sprintf.h stb_sprintf.c
copy ..\thirdparty\stb_image.h stb_image.c
copy ..\thirdparty\stb_truetype.h stb_truetype.c

cl -DSTB_IMAGE_WRITE_IMPLEMENTATION=1 -c -O2 -EHsc -nologo stb_image_write.c
cl -DSTB_SPRINTF_IMPLEMENTATION=1 -c -O2 -EHsc -nologo stb_sprintf.c
cl -DSTB_IMAGE_IMPLEMENTATION=1 -c -O2 -EHsc -nologo stb_image.c
cl -DSTB_TRUETYPE_IMPLEMENTATION=1 -c -O2 -EHsc -nologo stb_truetype.c

lib -nologo -out:stb_image_write.lib stb_image_write.obj
lib -nologo -out:stb_sprintf.lib stb_sprintf.obj
lib -nologo -out:stb_image.lib stb_image.obj
lib -nologo -out:stb_truetype.lib stb_truetype.obj

del stb_image_write.c
del stb_sprintf.c
del stb_image.c
del stb_truetype.c

popd 