#!/usr/bin/env sh

MYDIR="$(dirname ${0})"
cd "${MYDIR}"

# EASTL version 3.20.02
git subtree pull --squash --prefix=EASTL 'https://github.com/electronicarts/EASTL' 1aa784643081404783ce6494eb2fcaba99d8f6a5
git subtree pull --squash --prefix=EASTL/test/packages/EABase 'https://github.com/electronicarts/EABase' af4ae274795f4c40829428a2c4058c6f512530f4
