# Mingw64 Driver Plus Plus

A demonstration on how to compile Windows kernel drivers using Mingw64.

It provides also an example with a feature complete STL including your
beloved containers.

You will need an modern Mingw64-GCC toolchain.
Do not use any broken toolchains like the one shipped with debian-10.
Instead either use Zeranoe's build script with `make deps` or use your own.

## What?

1. ddk-template: plain and stupid ddk C example
2. ddk-template-cplusplus: same, but written in C++, including a very complex class and some MT
3. ddk-template-cplusplus-EASTL: C++ example w/ (EA)STL integration, basicially everything usable except for SEH and assertions.

## Build and Test

Build all examples with a Mingw64 toolchain using Zeranoe's build script:

``
make all
``

Build all examples with your own Mingw64 toolchain:

``
make all CC=path/to/bin/x86_64-w64-mingw32-gcc DDK_INCLUDE_DIR=path/to/include/ddk
``

Build Mingw64 only:

``
make deps
``

## Thanks!

- [Zeranoe](https://github.com/Zeranoe/mingw-w64-build) for the Mingw64 build script
- [sidyhe](https://github.com/sidyhe/dxx) for some copy paste ready CRT code ;)

and last but not least:

- [EA](https://github.com/electronicarts/EASTL), bad company, good STL
