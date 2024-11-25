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
