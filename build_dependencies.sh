#!/bin/bash
#
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
#
set -e
set -x
#############################
GITHUB_WORKSPACE="${PWD}"
ls -la ${GITHUB_WORKSPACE}
cd ${GITHUB_WORKSPACE}

############################# 
# Install Dependencies and packages

apt-get update -y -q

apt-get install -q -y cmake ninja-build git wget autoconf automake libtool pkg-config libdrm-dev libgbm-dev libgl1-mesa-dev libgles2-mesa-dev libegl1-mesa-dev libwayland-dev wayland-protocols libxkbcommon-dev libglib2.0-dev libsoup2.4-dev libboost-dev libjpeg-dev libpng-dev zlib1g-dev libsystemd-dev libexpat1-dev libffi-dev libxml2-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev freeglut3-dev libglew-dev rapidjson-dev meson xsltproc xmlto doxygen graphviz xutils-dev

############################
# Build Westeros
echo "======================================================================================"
echo "Building Westeros"

mkdir -p thirdparty
cd thirdparty

if [ ! -d "westeros" ]; then
    echo "Clone Westeros code"
    echo "=================================================================="
    git clone --depth 1 https://code.rdkcentral.com/r/components/opensource/westeros
    cd westeros
    if [ -f "${GITHUB_WORKSPACE}/scripts/westeros_ubuntu.patch" ]; then
        git apply "${GITHUB_WORKSPACE}/scripts/westeros_ubuntu.patch"
    fi
else
    cd westeros
fi

make -f Makefile.ubuntu
make install
echo "===============Westeros build complete====================="

cd ${GITHUB_WORKSPACE}
############################
