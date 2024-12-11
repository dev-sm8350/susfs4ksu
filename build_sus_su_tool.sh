#!/bin/bash

cat <<EOF >/dev/null
OLD_CWD=$(pwd)

if ! ndk-build -s -v &>/dev/null; then
    echo "[-] Have you added the root directory of ndk-build to your PATH envoironment variable?"
    exit 1
fi

set -x

cd ./sus_su
rm -rf libs obj 2>/dev/null
ndk-build
cp libs/arm64-v8a/sus_su ../ksu_module_susfs/tools/sus_su_arm64
cp libs/armeabi-v7a/sus_su ../ksu_module_susfs/tools/sus_su_arm
cd ${OLD_CWD}


EOF
