#!/bin/bash

OLD_CWD=$(pwd)

if ! ndk-build -s -v &>/dev/null; then
    echo "[-] Have you added the root directory of ndk-build to your PATH envoironment variable?"
    exit 1
fi

set -x

cd ./ksu_susfs
rm -rf libs obj 2>/dev/null
ndk-build
cp libs/arm64-v8a/ksu_susfs ../ksu_module_susfs/tools/ksu_susfs_arm64
cd ${OLD_CWD}



