![Gitlab-CI](https://gitlab.com/lnslbrty/mingw-w64-ddk-template/badges/master/pipeline.svg "Gitlab-CI: master branch")
![Circle-CI](https://circleci.com/gh/lnslbrty/mingw-w64-ddk-template.svg?style=shield "Circle-CI")

# Mingw64 Driver Plus Plus

A demonstration on how to compile Windows kernel drivers using Mingw64.

It provides also an example with a feature complete STL including your
beloved containers.

You will need an modern Mingw64-GCC toolchain.
Do not use any broken toolchains like the one shipped with debian-10.
Mingw64-GCC for debian-11 seems to work, but is not well tested.
Instead either use Zeranoe's build script with `make -C [path-to-this-repo] -f Makefile.deps all` or use your own.

## What?

1. ddk-template: plain and stupid ddk C example
2. ddk-template-cplusplus: same, but written in C++, including a very complex class and some MT
3. ddk-template-cplusplus-EASTL: C++ example w/ (EA)STL integration, basicially everything usable except for SEH and assertions.

## Build and Test

Build all examples with a Mingw64 toolchain using Zeranoe's build script:

```
make -C [path-to-this-repo] -f Makefile.deps all # build toolchain, CRT, CRT++ and EASTL
make -C [path-to-this-repo] all # build examples
```

Build all examples with your own Mingw64 toolchain:

``
make all CC=path/to/bin/x86_64-w64-mingw32-gcc CXX=path/to/bin/x86_64-w64-mingw32-g++ DDK_INCLUDE_DIR=path/to/include/ddk
``

## HowTo use it in your own project

At the moment only a **GMake** build system is supported.
A minimal working **Makefile** for your own project could look alike:

```make
ifndef DPP_ROOT
$(error DPP_ROOT is undefined)
endif

include $(DPP_ROOT)/Makefile.inc

DRIVER_NAME = Driver
DRIVER_OBJECTS = $(DRIVER_NAME).opp
DRIVER_TARGET = $(DRIVER_NAME).sys

%.opp: %.cpp
	$(call BUILD_CPP_OBJECT,$<,$@)

$(DRIVER_TARGET): $(DRIVER_OBJECTS)
	$(call LINK_CPP_KERNEL_TARGET,$(DRIVER_OBJECTS),$@)
```

Build it with: `make Driver.sys DPP_ROOT=[path/to/this/repository]`

It also possible to (self-)sign your driver and install your driver with:

```make
install: $(DRIVER_TARGET)
    $(call INSTALL_EXEC_SIGN,$(DRIVER_TARGET))
```

You can also add the toolchain to your path and use it for other projects w/o any Makefile blueprint:

```
make -C [path-to-this-repo] -f Makefile.deps all
source [path-to-this-repo]/w64-mingw32-sysroot/x86_64/activate.sh
```

## The CRT and CRT++

This project uses a very very rudimentary CRT for C and C++ projects.
Please keep in mind that depending on what you want to do the CRT may lack features you are familiar with.
Usually this will manifest in linker errors such as undefined references.
Most of the time copy&pasting missing libc/libgcc functions from various online sources should be sufficient.

Remember: The CRT/CRT++ **set a driver unload function** meaning that code .e.g.:

```C
NTSTATUS MyDriverEntry(_In_ struct _DRIVER_OBJECT * DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    DriverObject->DriverUnload = MyDriverUnload;
}
```

**must not** used. Overwriting `DriverObject->DriverUnload` with your own function may BSOD.
Instead the function `DriverUnload` will be called.
Make sure that the symbol `DriverUnload` exists and has the usual ddk function signature:
`void DriverUnload(_In_ struct _DRIVER_OBJECT * DriverObject)`.
This is required to make ctors/dtors work without calling additional functions in `DriverEntry` / `DriverUnload`.

## Host EASTL/CRT/CRT++ Build

It is possible to build parts of the repository for your host distribution.
To do that simply type:

``
make -C [path-to-this-repo] -f Makefile.deps -j1 all BUILD_NATIVE=1
``

You can use the Host Build in your Makefile based project with:

```
ifndef DPP_ROOT
$(error DPP_ROOT is undefined)
endif

ifndef BUILD_NATIVE
include $(DPP_ROOT)/Makefile.inc
else
include $(DPP_ROOT)/Makefile.native.inc
endif

DRIVER_NAME = Driver
DRIVER_OBJECTS = $(DRIVER_NAME).opp
DRIVER_TARGET = $(DRIVER_NAME).sys

USERSPACE_NAME = usa$(NAME_SUFFIX)
USERSPACE_OBJECTS = $(USERSPACE_NAME).opp
USERSPACE_TARGET = $(USERSPACE_NAME).exe

%.opp: %.cpp
	$(call BUILD_CPP_OBJECT,$<,$@)

$(DRIVER_TARGET): $(DRIVER_OBJECTS)
	$(call LINK_CPP_KERNEL_TARGET,$(DRIVER_OBJECTS),$@)

$(USERSPACE_TARGET): $(USERSPACE_OBJECTS)
	$(call LINK_CPP_USER_TARGET,$(USERSPACE_OBJECTS),$@)
```

## Thanks goes to:

- [Zeranoe](https://github.com/Zeranoe/mingw-w64-build) for the Mingw64 build script
- [sidyhe](https://github.com/sidyhe/dxx) for some copy paste ready CRT code
- [liupengs](https://github.com/liupengs/Mini-CRT) helped me to fix the ctor/dtor issue

and last but not least:

- [EA](https://github.com/electronicarts/EASTL), bad company, good STL
