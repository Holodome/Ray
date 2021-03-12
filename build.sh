#!/bin/bash

mkdir -p build
pushd ./build

build_options="-Wall -pedantic -pthread -Wno-writable-strings -lm -O0 -std=gnu11 -I../src"

clang -g $build_options -o ray ../src/ray.c

popd
