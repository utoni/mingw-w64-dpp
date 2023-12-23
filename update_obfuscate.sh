#!/usr/bin/env sh

MYDIR="$(dirname ${0})"
cd "${MYDIR}"

wget 'https://raw.githubusercontent.com/skadro-official/skCrypter/master/files/skCrypter.h' -O 'CRT/obfuscate.hpp'
