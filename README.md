[![Build](https://github.com/utoni/mingw-w64-dpp/actions/workflows/build.yml/badge.svg "Github Actions")](https://github.com/utoni/mingw-w64-dpp/actions/workflows/build.yml)
[![Gitlab-CI](https://gitlab.com/utoni/mingw-w64-ddk-template/badges/main/pipeline.svg "Gitlab-CI: main branch")](https://gitlab.com/utoni/mingw-w64-ddk-template/-/pipelines)
[![Circle-CI](https://circleci.com/gh/utoni/mingw-w64-dpp.svg?style=shield "Circle-CI")](https://app.circleci.com/pipelines/github/utoni/mingw-w64-dpp)

# Mingw64 Driver Plus Plus

A Windows kernel driver C/C++ SDK using Mingw64.

It provides also some examples and a feature complete STL including your
beloved containers.

You will need an modern Mingw64-GCC toolchain.
Do not use any broken toolchains like the one shipped with debian-10.
Mingw64 for debian-11 seems to work, but is not well tested. Mingw64 for debian-12 untested.
Instead either use Zeranoe's build script with `make -C [path-to-this-repo] -f Makefile.deps all` (same as `make -C [path-to-this-repo] deps`) or use your own.

## What?

1. `examples/dpp-template`: plain and stupid ddk C example
2. `examples/dpp-template-cplusplus`: same, but written in C++, including a very complex class and some MT
3. `examples/dpp-template-cplusplus-EASTL`: C++ example w/ (EA)STL integration, basicially everything usable except for VEH and assertions.

`examples/dpp-template-cplusplus-EASTL` supports `BUILD_NATIVE`!
You can build and run it on your native Linux either with the other examples e.g. `make examples`, build only native executables `make -C examples DPP_ROOT="$(realpath .)" BUILD_NATIVE=1` in the top-level directory or directly build it from the examples directory with `make DPP_ROOT="$(realpath ..)" BUILD_NATIVE=1`.

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
source [path-to-this-repo]/mingw-w64-sysroot/x86_64/activate.sh
```

### Packaging

It is possible to create `*.dpp` archives ready to get shipped.
To do this you need to call the `PACKAGE` macro.
But before you should install public header files with the `INSTALL_HEADERS` macro.

See the `Makefile` of [mingw-w64-ksocket](https://github.com/utoni/mingw-w64-ksocket.git) for an example.

## The CRT and CRT++

This project uses a very very rudimentary CRT for C and C++ projects.
Please keep in mind that depending on what you want to do the CRT may lack features you are familiar with.
Usually this will manifest in linker errors such as undefined references.
Most of the time copy&pasting missing libc/libgcc functions from various online sources should be sufficient.

Remember: The CRT/CRT++ **sets a driver unload function** meaning that code .e.g.:

```C
NTSTATUS DriverEntry(_In_ struct _DRIVER_OBJECT * DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    DriverObject->DriverUnload = MyDriverUnload;
}
```

**must not** used. Overwriting `DriverObject->DriverUnload` with your own function may BSOD.
Instead the function `DriverUnload` will be called.
Make sure that the symbol `DriverUnload` exists and has the usual ddk function signature:
`void DriverUnload(_In_ struct _DRIVER_OBJECT * DriverObject)`.
This is required to make ctors/dtors work without calling additional functions in `DriverEntry` / `DriverUnload`.

Do not forget to disable `C++ name mangeling` if your driver source which contains the `DriverEntry` and `DriverUnload` symbols is compiled with `g++`:

```C++
extern "C" {
NTSTATUS DriverEntry(_In_ struct _DRIVER_OBJECT *DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    // ...
}
VOID DriverUnload(_In_ struct _DRIVER_OBJECT *DriverObject)
{
    // ...
}
}
```

## Host EASTL/CRT/CRT++ Build

It is possible to build parts of the repository for your host distribution.
To do that simply type:

``
make -C [path-to-this-repo] -f Makefile.deps -j1 all BUILD_NATIVE=1
``

The results should be visible in `./CRT` and `EASTL-native-build`.
If you ran `make -C [path-to-this-repo] deps` before, everything is already done including the native build.

You can use the Host Build in your Makefile based project with:

```make
ifndef DPP_ROOT
$(error DPP_ROOT is undefined)
endif

ifndef BUILD_NATIVE
include $(DPP_ROOT)/Makefile.inc
else
include $(DPP_ROOT)/Makefile.native.inc
endif

# Driver
DRIVER_NAME = Driver
DRIVER_OBJECTS = $(DRIVER_NAME).opp
DRIVER_TARGET = $(DRIVER_NAME).sys
DRIVER_LIBS =
CFLAGS_$(DRIVER_NAME).opp =
LDFLAGS_$(DRIVER_NAME).sys =

# Userspace
USER_NAME = usa$(NAME_SUFFIX)
USER_OBJECTS = $(USER_NAME).opp
USER_TARGET = $(USER_NAME).exe
USER_LIBS =
CFLAGS_$(USER_NAME).opp =
LDFLAGS_$(USER_NAME).exe =

# specify additional CFLAGS for kernel/user targets
CUSTOM_CFLAGS = -I.

%.opp: %.cpp
	$(call BUILD_CPP_OBJECT,$<,$@)

$(DRIVER_TARGET): $(DRIVER_OBJECTS)
	$(call LINK_CPP_KERNEL_TARGET,$(DRIVER_OBJECTS),$@)

$(USER_TARGET): $(USER_OBJECTS)
	$(call LINK_CPP_USER_TARGET,$(USER_OBJECTS),$@)
```

[A simple and stupid project example.](https://github.com/utoni/mingw-w64-driver)

## Driver Signing

Driver signing can be done in two ways. Using a native `osslsigncode` executable or sign it manually on your Windows platform.
The first one is always done by calling the macro `INSTALL_EXEC_SIGN` from your own Makefile.
The latter one has to be done manually on your target Windows machine by running:

1. `create_codesign_ca.bat` in DESTDIR (Administrator permission required to import CA/CERTs to the Windows certificate storage
2. `*-sign-driver-on-windows.bat` e.g. `dpp-example-sign-driver-on-windows.bat` (no Administrator permissions required)

*Note*: You still need to call the macro `INSTALL_EXEC_SIGN` from your own Makefile to create/install the batch files in DESTDIR.

## Extending

You may reach a point where system functions, intrinsics or built-ins are required.
For system functions that can be retrieved via `MmGetSystemRoutineAddress`, you may use `CRT/gen_wrapper.sh` to create wrapper modules. These modules retrieve the desired functions during run-time, but will be available during link-time. An example is `ZwTraceControl`, which gets exported by `ntdll.dll`.
Eventually missing intrinsics/built-ins should be placed in `CRT/kcrt.c`.

## Notes on SEH

There are some limitations regarding the use of MingW's SEH implementation in kernel mode.
You can not use it as you might have been known it from MSVC or MingW.

Predefined statements `__try`, `__catch`, `__finally`, `__try1`, `__catch1`, etc. do **not** work!

You need to use something like:

```C
#include <except.h>

int handler(_In_ EXCEPTION_POINTERS * ep)
{
    (void)ep;
    return EXCEPTION_EXECUTE_HANDLER;
}

// ...

__dpptry(handler, unique_identifier) {
    *(int *)0 = 0;
} __dppexcept(unique_identifier) {
    DbgPrint("%s\n", "Exception caught!");
} __dpptryend(unique_identifier);
```

SEH needs to be improved. Patches are welcome!

## Known Issues

Unfortunately, `eastl::to_string` does not work. See `CRT/eastl_compat.cpp` for more details.
Instead, `#include <eastl_compat.hpp>` and use `::to_string` globals.

## Thanks goes to:

- [Zeranoe](https://github.com/Zeranoe/mingw-w64-build) for the Mingw64 build script
- [sidyhe](https://github.com/sidyhe/dxx) for some copy paste ready CRT code
- [liupengs](https://github.com/liupengs/Mini-CRT) helped me to fix the ctor/dtor issue

and last but not least:

- [EA](https://github.com/electronicarts/EASTL), bad company, good STL
