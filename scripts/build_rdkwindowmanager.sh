#!/bin/bash

# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2024 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -e
echo "Platform to be run on :$1"

if [[ "$1" == "ubuntu" ]]; then
    echo "Building for ubuntu..."
else
    echo "Error: $1 is not supported"
    exit 1
fi

echo "Install dependent packages"
echo "=================================================="
mkdir thirdparty
cd thirdparty

if [ ! -d "westeros" ]; then
    echo "Clone Westeros code"
    echo "=================================================================="
    git clone https://code.rdkcentral.com/r/components/opensource/westeros
    cd westeros
    git apply ../../rdk-window-manager/scripts/westeros_ubuntu.patch
fi
make -f Makefile.ubuntu
make install
echo "===============Westeros build complete====================="

cd ../../

echo "===============RDK-window-manager build start====================="
export  LIB_PATH=$PWD/thirdparty/westeros/external/install/lib/
cmake -DINCLUDE_HEADER_DIR=$PWD/thirdparty/westeros/external/install/include -S rdk-window-manager -B build/rdk-window-manager
cmake --build build/rdk-window-manager
echo "===============RDK-window-manager build complete====================="
