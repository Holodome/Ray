#!/bin/bash

mkdir -p build
<<<<<<< HEAD
pushd ./build 

if [ ! -f ./stb_sprintf.o ];
then 
    cp ../thirdparty/stb_image_write.h stb_image_write.c
    cp ../thirdparty/stb_sprintf.h stb_sprintf.c
    cp ../thirdparty/stb_image.h stb_image.c
    cp ../thirdparty/stb_truetype.h stb_truetype.c
    
    clang -O3 -DSTB_IMAGE_WRITE_IMPLEMENTATION=1 -c -o stb_image_write.o stb_image_write.c
    clang -O3 -DSTB_SPRINTF_IMPLEMENTATION=1 -c -o stb_sprintf.o stb_sprintf.c
    clang -O3 -DSTB_IMAGE_IMPLEMENTATION=1 -c -o stb_image.o stb_image.c
    clang -O3 -DSTB_TRUETYPE_IMPLEMENTATION=1 -c -o stb_truetype.o stb_truetype.c
    
    rm stb_image_write.c
    rm stb_sprintf.c
    rm stb_image.c
    rm stb_truetype.c
fi 

RayDisabledErrors="-Wall -Wno-writable-strings -Wno-unused-function -Wno-unused-variable -Wparentheses -Wswitch -Wno-gnu-designator"
RayLinkedLibraries="-L./ -l:stb_sprintf.o -l:stb_image.o -l:stb_truetype.o -l:stb_image_write.o"
RayCompilerSwitches="-D_CRT_SECURE_NO_WARNINGS"
RayBuildOptions="$RayCompilerSwitches -lm -pthread -O3 -I../ray -I../thirdparty $RayDisabledErrors $RayLinkedLibraries -std=c99 -fno-exceptions -fno-math-errno"

clang -g $RayBuildOptions -o ray ../ray/ray/ray.c 

popd
=======
pushd ./build

build_options="-Wall -pedantic -pthread -Wno-writable-strings -lm -O0 -std=gnu11 -I../src"

clang -g $build_options -o ray ../src/ray.c

popd
>>>>>>> remotes/origin/better_ray
