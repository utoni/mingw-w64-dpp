#!/usr/bin/env sh

MYDIR="$(dirname ${0})"
cd "${MYDIR}"

git subtree pull --squash --prefix=mingw-w64-build 'https://github.com/utoni/mingw-w64-build-ng.git' main
