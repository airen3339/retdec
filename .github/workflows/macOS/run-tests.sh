#!/usr/bin/bash

cat <<EOF > $PWD/retdec-regression-tests-framework/config_local.ini
[runner]
; Path to the extracted Clang package containing subdirectories such as bin, include, lib, share.
clang_dir = $PWD/clang
; Path to the cloned repository containing regression tests.
tests_root_dir = $PWD/retdec-regression-tests
; Path to the RetDec's installation directory.
retdec_install_dir = $PWD/install
EOF

cd "$PWD/retdec-regression-tests-framework"

python3 -m venv .venv

. .venv/bin/activate
pip3 install -r requirements.txt

python3 ./runner.py
