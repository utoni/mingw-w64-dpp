#!/usr/bin/env sh

MYDIR="$(dirname ${0})"
cd "${MYDIR}"

wget 'https://raw.githubusercontent.com/charlesnicholson/nanoprintf/main/nanoprintf.h' -O 'CRT/nanoprintf.h'
