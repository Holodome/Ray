#!/bin/bash

mkdir -p build
pushd ./build 

build_options="-Wall -pedantic -Wno-writable-strings -lm -O3 -I../src"

clang -g $build_options -o ray ../src/main.c

popd