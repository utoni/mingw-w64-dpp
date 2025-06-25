#!/usr/bin/env sh

MYDIR="$(dirname ${0})"
cd "${MYDIR}"

git subtree pull --squash --prefix=EASTL 'https://github.com/electronicarts/EASTL' e045e9d11fe4c9c7cc0e387ea6410524d4dce0d3
git subtree pull --squash --prefix=EASTL/test/packages/EABase 'https://github.com/electronicarts/EABase' af4ae274795f4c40829428a2c4058c6f512530f4
