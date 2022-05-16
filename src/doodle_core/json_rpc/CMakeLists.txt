
add_subdirectory(core)
add_subdirectory(exception)
target_sources(doodle_core
        PUBLIC
        json_rpc_client.h
        json_rpc_server.h
        json_rpc_static_value.h

        PRIVATE
        json_rpc_client.cpp
        json_rpc_server.cpp
        )
