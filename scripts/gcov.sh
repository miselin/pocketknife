#!/bin/bash

BASE_PATH=$(dirname -- $(readlink -f -- "$0"))
SOURCE_ROOT=$(dirname -- "$BASE_PATH")

OUT_DIR=${SOURCE_ROOT}/build/coverage
mkdir -p ${OUT_DIR}

gcovr ${SOURCE_ROOT}/build -r ${SOURCE_ROOT}/src --html-details=${OUT_DIR}/coverage.html --txt "$@"
