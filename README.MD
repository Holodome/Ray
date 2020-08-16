# Ray

Tiny CPU raytracer made in C11 for educational purposes

## Structure 

Shippable version consists of several files.
ray.exe is a command-line program that performs raytracing and saves it to dest file
TODO add file to arguments
TODO loading scenes from some format
editor.exe is a graphics application that provides user-accesible way of setting up scenes 
for raytracer

Whether to make own scene format or pick ready is still not decided. Making own format will be better, since in many ways this is not commerical raytracer and scenes are much simpler.
TODO importing materials from different formats. See BRDF databases

TODO have dlls of different versions of raytracer depending on processor characteristics (plain, SSE, AVX, AVX512(??))

## Compilation 

Compilation is similar on all platforms: run build scripts, it will compile and put everything in build directory.

ON WINDOWS:
run build.bat
ON MACOS:
run build.sh

On windows LLVM is used to produce binary, not MSVC. And since LLVM is aviable on all other platforms, program makes use of some of clang's language extensions that make programming much more saner. Current version of code cannot be compiled with MSVC.

Currently compilation in done in what is called 'Unity build' - it consists of only one translation unit. This approach is often said to be bad, but it makes compile times much faster and is easy to maintain in such small project.
If project grows much more complicated, it is possible to integrate some build system, like cmake and switch to incremental build


## Some screenshots

> Note that raytracer is still early in development, so screenshots are not of the best possible quality 

![Screenshot1](screenshots/0-1024rp.jpg)
![Screenshot2](screenshots/1-128.jpg)
![Screenshot3](screenshots/cornellbox1-8192rp-42m.png)