#!/usr/bin/env sh

set -e
set -x

COPT='-algorithm EC  -pkeyopt ec_paramgen_curve:secp384r1 -pkeyopt ec_param_enc:named_curve'
MYDIR="$(dirname "${0}")"

readonly BASE="${1:-ddk}"
readonly REVI=1
readonly PRFX="${BASE}-"
readonly ROOT="${PRFX}ca"
readonly CODE="${PRFX}code"

cd "${MYDIR}"

# PKCS #8 private key, encrypted, PEM format.
# shellcheck disable=SC2086
openssl genpkey ${COPT} -aes-256-cbc -pass "pass:onlytemporary" -out "${ROOT}-private.pem" 2>/dev/null
openssl ec -in "${ROOT}-private.pem" -passin "pass:onlytemporary" -out "${ROOT}-private-nopass.pem" #2>/dev/null
mv "${ROOT}-private-nopass.pem" "${ROOT}-private.pem"
openssl asn1parse -i -in "${ROOT}-private.pem"

# -cert.pem is certificate (public key + subject + signature)
openssl req -batch -verbose -new -sha256 -x509 -days 1826 -key "${ROOT}-private.pem" -out "${ROOT}-cert.pem" -config - << EOF
[req]
encrypt_key = yes
prompt = no
utf8 = yes
string_mask = utf8only
distinguished_name = dn
x509_extensions = v3_ca

[v3_ca]
subjectKeyIdentifier = hash
basicConstraints = critical, CA:TRUE, pathlen:0
keyUsage = critical, keyCertSign, cRLSign

[dn]
CN = ${BASE} Root CA ${REVI}
EOF
openssl x509 -in "${ROOT}-cert.pem" -text -noout -nameopt utf8 -sha256 -fingerprint > "${ROOT}-cert.pem.x509.txt"
openssl asn1parse -i -in "${ROOT}-cert.pem" > "${ROOT}-cert.pem.asn1.txt"

# subordinate #1: code signing
cat << EOF > "${CODE}-csr.config"
[req]
encrypt_key = yes
prompt = no
utf8 = yes
string_mask = utf8only
distinguished_name = dn
req_extensions = v3_req

[v3_req]
subjectKeyIdentifier = hash
keyUsage = critical, digitalSignature
# msCodeInd = Microsoft Individual Code Signing
# msCodeCom = Microsoft Commercial Code Signing
extendedKeyUsage = critical, codeSigning, msCodeInd

[dn]
CN = ${base} Code Signing Authority
EOF

# PKCS #8 private key, encrypted, PEM format.
# shellcheck disable=SC2086
openssl genpkey ${COPT} -aes-256-cbc -pass "pass:onlytemporary" -out "${CODE}-private.pem" 2>/dev/null
openssl ec -in "${CODE}-private.pem" -passin "pass:onlytemporary" -out "${CODE}-private-nopass.pem" 2>/dev/null
mv "${CODE}-private-nopass.pem" "${CODE}-private.pem"
openssl asn1parse -i -in "${CODE}-private.pem"

openssl pkey -in "${CODE}-private.pem" -pubout > "${CODE}-public.pem"
# Play some with the public key
openssl pkey -pubin -in "${CODE}-public.pem" -text -noout > "${CODE}-public.pem.txt"
openssl asn1parse -i -in "${CODE}-public.pem" > "${CODE}-public.pem.asn1.txt"

# -csr.pem is certificate signing request
openssl req -batch -verbose -new -sha256 -key "${CODE}-private.pem" -out "${CODE}-csr.pem" -config "${CODE}-csr.config"
openssl req -batch -verbose -in "${CODE}-csr.pem" -text -noout -nameopt utf8 > "${CODE}-csr.pem.txt"
openssl asn1parse -i -in "${CODE}-csr.pem" > "${CODE}-csr.pem.asn1.txt"

# -cert.pem is certificate (public key + subject + signature)
openssl x509 -req -sha256 -days 1095 \
  -extfile "${CODE}-csr.config" -extensions v3_req \
  -in "${CODE}-csr.pem" \
  -CA "${ROOT}-cert.pem" -CAkey "${ROOT}-private.pem" -CAcreateserial -out "${CODE}-cert.pem"
openssl x509 -in "${CODE}-cert.pem" -text -noout -nameopt utf8 -sha256 -fingerprint > "${CODE}-cert.pem.x509.txt"
openssl asn1parse -i -in "${CODE}-cert.pem" > "${CODE}-cert.pem.asn1.txt"

# PKCS #12 .p12 is private key and certificate(-chain), encrypted
openssl pkcs12 -export \
  -keypbe aes-256-cbc -certpbe aes-256-cbc -macalg sha256 \
  -inkey "${CODE}-private.pem" \
  -in "${CODE}-cert.pem" \
  -chain -CAfile "${ROOT}-cert.pem" \
  -out "${CODE}.p12"
openssl pkcs12 -in "${CODE}.p12" -info -nodes -nokeys -out "${CODE}.p12.txt"
openssl asn1parse -i -inform DER -in "${CODE}.p12"
