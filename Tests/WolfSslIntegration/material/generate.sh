#!/usr/bin/env bash
#
# Generates the test-only certificate material the wolfSSL stream integration
# tests present to the stream. Run at build time into the build tree, never
# committed: neither wolfSSL build in the image can issue a certificate, and the
# OpenSSL command-line tool can, including into a validity window that has
# closed or not yet opened.
#
# Usage: generate.sh <output directory>
#
# One authority is the trust anchor; a second exists so a leaf can fail to
# chain. Each server certificate carries exactly one fault except the two that
# carry two, which prove the order the refusal names them in. The self-signed
# ones are the collectors a pin authorises with no chain at all, and every
# server certificate has its pins written beside it, computed here rather than
# by the library under test.

set -euo pipefail

out=$1
mkdir -p "$out"
cd "$out"

DAYS=3650
NAME=syslog.example.com
SAN_NAME="subjectAltName = DNS:$NAME"
SAN_NAME_AND_ADDRESS="subjectAltName = DNS:$NAME, IP:127.0.0.1"
SAN_OTHER="subjectAltName = DNS:other.example.com"

# Absolute windows for the certificates meant to be outside one.
EXPIRED_FROM=20200101000000Z
EXPIRED_TO=20200102000000Z
FUTURE_FROM=20900101000000Z
FUTURE_TO=20900102000000Z

# $1 = basename, $2 = subject, $3 = key bits.
make_csr() {
    openssl genrsa -out "$1.key" "$3" 2>/dev/null
    openssl req -new -key "$1.key" -subj "$2" -out "$1.csr"
}

# $1 = basename, $2 = issuer basename, $3 = extension text.
sign_leaf() {
    openssl x509 -req -in "$1.csr" -CA "$2.pem" -CAkey "$2.key" -CAcreateserial \
        -out "$1.pem" -days "$DAYS" -sha256 \
        -extfile <(printf "%s\n" "$3") 2>/dev/null
}

# `openssl x509 -req` cannot set a validity window before OpenSSL 3.5, so these
# go through `ca`, which can, with the database directory it insists on.
# $1 = basename, $2 = issuer basename, $3 = extension text, $4 = notBefore, $5 = notAfter.
sign_leaf_between() {
    local db="cadb-$1"
    mkdir -p "$db/newcerts"
    : > "$db/index.txt"
    echo 01 > "$db/serial"
    openssl ca -batch \
        -config <(printf '%s\n' \
            "[ca]" "default_ca = CA_default" "[CA_default]" \
            "database = $db/index.txt" "new_certs_dir = $db/newcerts" "serial = $db/serial" \
            "default_md = sha256" "policy = policy_any" "email_in_dn = no" \
            "rand_serial = no" "unique_subject = no" \
            "[policy_any]" "commonName = supplied") \
        -cert "$2.pem" -keyfile "$2.key" \
        -startdate "$4" -enddate "$5" \
        -extfile <(printf "%s\n" "$3") \
        -in "$1.csr" -out "$1.pem" -notext 2>/dev/null
    rm -rf "$db"
}

# $1 = basename, $2 = extension text. Signed with its own key.
self_sign() {
    openssl x509 -req -in "$1.csr" -signkey "$1.key" -out "$1.pem" -days "$DAYS" -sha256 \
        -extfile <(printf "%s\n" "$2") 2>/dev/null
}

# As sign_leaf_between, signed with its own key.
# $1 = basename, $2 = extension text, $3 = notBefore, $4 = notAfter.
self_sign_between() {
    local db="cadb-$1"
    mkdir -p "$db/newcerts"
    : > "$db/index.txt"
    echo 01 > "$db/serial"
    openssl ca -batch -selfsign \
        -config <(printf '%s\n' \
            "[ca]" "default_ca = CA_default" "[CA_default]" \
            "database = $db/index.txt" "new_certs_dir = $db/newcerts" "serial = $db/serial" \
            "default_md = sha256" "policy = policy_any" "email_in_dn = no" \
            "rand_serial = no" "unique_subject = no" \
            "[policy_any]" "commonName = supplied") \
        -keyfile "$1.key" \
        -startdate "$3" -enddate "$4" \
        -extfile <(printf "%s\n" "$2") \
        -in "$1.csr" -out "$1.pem" -notext 2>/dev/null
    rm -rf "$db"
}

# The RFC 5425 section 4.2.2 pins of the leaf in $1.pem, one file per hash.
write_pins() {
    local sha256 sha1
    sha256=$(openssl x509 -in "$1.pem" -noout -fingerprint -sha256 | cut -d= -f2)
    sha1=$(openssl x509 -in "$1.pem" -noout -fingerprint -sha1 | cut -d= -f2)
    printf 'sha-256:%s' "$sha256" > "$1.sha256"
    printf 'sha-1:%s' "$sha1" > "$1.sha1"
}

openssl genrsa -out ca.key 2048 2>/dev/null
openssl req -x509 -new -nodes -key ca.key -sha256 -days "$DAYS" \
    -subj "/CN=SolidSyslog wolfSSL Test CA" -out ca.pem

openssl genrsa -out ca-b.key 2048 2>/dev/null
openssl req -x509 -new -nodes -key ca-b.key -sha256 -days "$DAYS" \
    -subj "/CN=SolidSyslog wolfSSL Test CA B" -out ca-b.pem

# The collector on the happy path: named by DNS only.
make_csr server "/CN=$NAME" 2048
sign_leaf server ca "$SAN_NAME"

# The same, also naming its address, for the build that can match one.
make_csr server-ip "/CN=$NAME" 2048
sign_leaf server-ip ca "$SAN_NAME_AND_ADDRESS"

# Right name, wrong issuer.
make_csr server-untrusted "/CN=$NAME" 2048
sign_leaf server-untrusted ca-b "$SAN_NAME"

# Right issuer, wrong name.
make_csr server-wrongname "/CN=other.example.com" 2048
sign_leaf server-wrongname ca "$SAN_OTHER"

# Right issuer and name, window closed.
make_csr server-expired "/CN=$NAME" 2048
sign_leaf_between server-expired ca "$SAN_NAME" "$EXPIRED_FROM" "$EXPIRED_TO"

# Right issuer and name, window not yet open.
make_csr server-notyetvalid "/CN=$NAME" 2048
sign_leaf_between server-notyetvalid ca "$SAN_NAME" "$FUTURE_FROM" "$FUTURE_TO"

# Two faults: untrusted issuer and a closed window.
make_csr server-untrusted-expired "/CN=$NAME" 2048
sign_leaf_between server-untrusted-expired ca-b "$SAN_NAME" "$EXPIRED_FROM" "$EXPIRED_TO"

# Two faults: wrong name and a closed window.
make_csr server-wrongname-expired "/CN=other.example.com" 2048
sign_leaf_between server-wrongname-expired ca "$SAN_OTHER" "$EXPIRED_FROM" "$EXPIRED_TO"

# Sound in every respect but the key, which is below the BCP 195 floor.
make_csr server-weak "/CN=$NAME" 1024
sign_leaf server-weak ca "$SAN_NAME"

# A collector with no PKI: self-signed, sound in every respect.
make_csr server-selfsigned "/CN=$NAME" 2048
self_sign server-selfsigned "$SAN_NAME"

# Self-signed, window closed.
make_csr server-selfsigned-expired "/CN=$NAME" 2048
self_sign_between server-selfsigned-expired "$SAN_NAME" "$EXPIRED_FROM" "$EXPIRED_TO"

# Self-signed, window not yet open.
make_csr server-selfsigned-notyetvalid "/CN=$NAME" 2048
self_sign_between server-selfsigned-notyetvalid "$SAN_NAME" "$FUTURE_FROM" "$FUTURE_TO"

# Self-signed, wrong name.
make_csr server-selfsigned-wrongname "/CN=other.example.com" 2048
self_sign server-selfsigned-wrongname "$SAN_OTHER"

# Self-signed, key below the BCP 195 floor.
make_csr server-selfsigned-weak "/CN=$NAME" 1024
self_sign server-selfsigned-weak "$SAN_NAME"

# The device's own credential, for a collector that asks for one.
make_csr client "/CN=device.example.com" 2048
sign_leaf client ca "basicConstraints = CA:FALSE"

# The happy-path collector presenting its issuer behind it.
cat server.pem ca.pem > server-chain.pem
cp server.key server-chain.key

for certificate in server*.pem; do
    write_pins "${certificate%.pem}"
done

rm -f ./*.csr ./*.srl
touch generated.stamp
