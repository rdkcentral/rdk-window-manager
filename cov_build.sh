#!/bin/bash
# Copyright (c) 2024 RDK Management
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
set -x
set -e
##############################
GITHUB_WORKSPACE="${PWD}"
ls -la "${GITHUB_WORKSPACE}"

# Exclude non-product directories from the build source tree so the scan prep
# are run against a clean product build without those folders. Keep the build
# rooted outside the repo checkout to avoid CMake generating include paths that
# point back into the original source directory.
COV_BUILD_ROOT="${RUNNER_TEMP:-${TMPDIR:-/tmp}}"
COV_BUILD_SOURCE="${COV_BUILD_ROOT}/rdk-window-manager-cov"
rm -rf "${COV_BUILD_SOURCE}"
mkdir -p "${COV_BUILD_SOURCE}"

echo "DEBUG: repo root: ${GITHUB_WORKSPACE}"
echo "DEBUG: using external filtered source tree for scan prep: ${COV_BUILD_SOURCE}"
find "${GITHUB_WORKSPACE}" -mindepth 1 -maxdepth 1 \
    ! -name '.git' \
    ! -name '.cov_build_source' \
    ! -name 'build' \
    ! -name 'tests' \
    -exec cp -a {} "${COV_BUILD_SOURCE}/" \;

echo "DEBUG: excluded directories: .git .cov_build_source build tests; keeping install for dependency resolution"

echo "DEBUG: disabling test-app targets in scan-prep build because the excluded tests directory is intentionally not present"

cd "${COV_BUILD_SOURCE}"

############################
# Build rdk-window-manager
echo "======================================================================================"
echo "building rdk-window-manager"

export LIB_PATH="${COV_BUILD_SOURCE}/thirdparty/westeros/external/install/lib/"
cmake -DINCLUDE_HEADER_DIR="${COV_BUILD_SOURCE}/thirdparty/westeros/external/install/include" \
    -DRDK_WINDOW_MANAGER_BUILD_TEST_APP=OFF \
    -S . -B build
cmake --build build -j $(nproc)
echo "======================================================================================"

