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
cd "${GITHUB_WORKSPACE}"

############################
# Build rdk-window-manager
echo "======================================================================================"
echo "building rdk-window-manager"

set --
EXCLUDED_PATHS=""
for coverity_path in \
    "${GITHUB_WORKSPACE}/tests"; do
    if [ -d "${coverity_path}" ]; then
        set -- "$@" --exclude-path "$coverity_path"
        if [ -n "${EXCLUDED_PATHS}" ]; then
            EXCLUDED_PATHS="${EXCLUDED_PATHS} ${coverity_path}"
        else
            EXCLUDED_PATHS="${coverity_path}"
        fi
    fi
done

echo "Coverity exclude paths: ${EXCLUDED_PATHS}"

export LIB_PATH="${GITHUB_WORKSPACE}/thirdparty/westeros/external/install/lib/"
cmake -DINCLUDE_HEADER_DIR="${GITHUB_WORKSPACE}/thirdparty/westeros/external/install/include" -S . -B build
if command -v cov-build >/dev/null 2>&1; then
    cov-build --dir "${GITHUB_WORKSPACE}/cov-int" "$@" -- cmake --build build -j $(nproc)
else
    cmake --build build -j $(nproc)
fi
echo "======================================================================================"

