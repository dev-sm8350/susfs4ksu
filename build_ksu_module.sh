#!/bin/bash

OUT_FILE=ksu_module_susfs.zip
OLD_CWD=$(pwd)

cd ksu_module_susfs && zip -r9 ../${OUT_FILE} * -x ${OUT_FILE}

cd ${OLD_CWD}
