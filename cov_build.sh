#!/bin/bash
set -x
set -e
##############################
GITHUB_WORKSPACE="${PWD}"
ls -la ${GITHUB_WORKSPACE}
cd ${GITHUB_WORKSPACE}

############################
# Build rdk-window-manager
echo "======================================================================================"
echo "building rdk-window-manager"

export LIB_PATH=${GITHUB_WORKSPACE}/thirdparty/westeros/external/install/lib/
cmake -DINCLUDE_HEADER_DIR=${GITHUB_WORKSPACE}/thirdparty/westeros/external/install/include -S . -B build
cmake --build build -j $(nproc)
echo "======================================================================================"
