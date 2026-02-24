# mingw-w64-build-ng
[Zeranoe](https://github.com/Zeranoe/mingw-w64-build)s build script with some extensions rejected by the upstream.

[![Build](https://github.com/utoni/mingw-w64-build-ng/actions/workflows/build.yml/badge.svg "Github Actions")](https://github.com/utoni/mingw-w64-build-ng/actions/workflows/build.yml)
[![Gitlab-CI](https://gitlab.com/utoni/mingw-w64-build-ng/badges/main/pipeline.svg "Gitlab-CI: main branch")](https://gitlab.com/utoni/mingw-w64-build-ng/-/pipelines)

# mingw-w64-build
mingw-w64-build is a Bash script to build a [MinGW-w64](https://mingw-w64.org)
cross compiler for i686 (Win32) and x86_64 (Win64). It will build a fully static
toolchain that can compile Windows executables that don't depend on any GCC dll
files.

## Default Branches
* [MinGW-w64](https://mingw-w64.org) master
* [Binutils](https://www.gnu.org/software/binutils/) binutils-2_42-branch
* [GCC](https://gcc.gnu.org/) releases/gcc-14

## Default Prefix
`$HOME/.mingw-w64-build-ng/i686` and `$HOME/.mingw-w64-build-ng/x86_64` are the
default install locations, but this location can be modified with the `--prefix`
option. To ensure the new compilers are available system-wide, add
`$HOME/.mingw-w64-build-ng/<arch>/bin` to the `$PATH`.

## Platforms
mingw-w64-build should run on Ubuntu, Cygwin, macOS (with Homebrew), and other
Bash based shells.

## Usage
See `mingw-w64-build --help` for all build options.

## License
mingw-w64-build is licensed under the GNU GPL 3.0 or later. A copy of the
license can be found in the LICENSE file.
