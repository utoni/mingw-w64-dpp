#!/usr/bin/env sh

MYDIR="$(dirname ${0})"
cd "${MYDIR}"

git subtree pull --squash --prefix=EASTL 'https://github.com/electronicarts/EASTL' master
git subtree pull --squash --prefix=EASTL/test/packages/EABase 'https://github.com/electronicarts/EABase' master
