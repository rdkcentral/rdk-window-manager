add_executable(clientfirebolt
        clientFirebolt.cpp
)

target_link_libraries(clientfirebolt -lwayland-client)

set_target_properties(clientfirebolt
        PROPERTIES
        OUTPUT_NAME firebolt_extension_test
)
