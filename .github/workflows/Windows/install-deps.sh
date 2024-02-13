#!/usr/bin/bash

rm -rf "/c/Program Files/OpenSSL"

choco install --no-progress openssl 7zip

# Chocolatey installs OpenSSL into subfolder MD which isn't found with
# FindOpenSSL.cmake in the version we have available. This moves the files
# where the finder expects them. Otherwise mismatch between OpenSSL versions
# can and will happen.
set -x
ls -la "/c/Program Files/"
ls -la "/c/Program Files/OpenSSL/"
ls -la "/c/Program Files/OpenSSL/lib/"
ls -la "/c/Program Files/OpenSSL/lib/VC/"
ls -la "/c/Program Files/OpenSSL/lib/VC/x64/"
ls -la "/c/Program Files/OpenSSL/lib/VC/x64/MD/"
cp "/c/Program Files/OpenSSL/lib/VC/x64/MD/*" "/c/Program Files/OpenSSL/lib/VC/x64/"
