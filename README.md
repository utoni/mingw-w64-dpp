# Mingw64 Driver Plus Plus

A demonstration on how to compile Windows kernel drivers using Mingw64.

It provides also an example with a feature complete STL including your
beloved containers.

You will need an modern Mingw64-GCC toolchain.
Do not use any broken toolchains like the one shipped with debian-10.
Instead either use Zeranoe's build script with `make deps` or use your own.

## What?

1. ddk-template: plain and stupid ddk C example
2. ddk-template-cplusplus: same, but written in C++, including a very complex class
3. ddk-template-cplusplus: C++ example w/ (EA)STL integration, everything usable except for Threads

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
- [sidhye](https://github.com/sidhye/dxx) for some copy paste ready CRT code ;)
