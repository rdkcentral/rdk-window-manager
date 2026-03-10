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

message("Building L1 Tests...")

# Define base directory for headers
set(BASEDIR ${CMAKE_CURRENT_SOURCE_DIR}/Tests)
set(EMPTY_HEADERS_DIRS
    ${BASEDIR}
    ${BASEDIR}/headers
)

# Define the empty headers to be generated
set(EMPTY_HEADERS
    #${BASEDIR}/headers/wayland-client.h
    #${BASEDIR}/headers/wayland-server-core.h
	${BASEDIR}/headers/wayland-server-protocol.h
	#${BASEDIR}/headers/rdkwindowmanagerdata.h
	#${BASEDIR}/headers/rdkwindowmanagerevents.h
	${BASEDIR}/headers/rdkcompositor.h
	${BASEDIR}/headers/wayland-version.h
	${BASEDIR}/headers/wayland-client-protocol.h
	#${BASEDIR}/headers/inputevent.h
	#${BASEDIR}/headers/application.h
	#${BASEDIR}/headers/rdkwindowmanagerrect.h
	#${BASEDIR}/headers/rdkwindowmanagertypes.h
)

# Create the directories if they don't exist
file(MAKE_DIRECTORY ${EMPTY_HEADERS_DIRS})

# Generate the empty headers if they don't already exist
file(GLOB_RECURSE EMPTY_HEADERS_AVAILABLE "${BASEDIR}/*")
if (EMPTY_HEADERS_AVAILABLE)
    message("Skip already generated headers to avoid rebuild")
    list(REMOVE_ITEM EMPTY_HEADERS ${EMPTY_HEADERS_AVAILABLE})
endif ()
if (EMPTY_HEADERS)
    file(TOUCH ${EMPTY_HEADERS})
endif ()

include_directories(${EMPTY_HEADERS_DIRS})
