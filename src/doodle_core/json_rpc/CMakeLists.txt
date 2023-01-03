
add_subdirectory(core)
add_subdirectory(exception)
add_subdirectory(args)
target_sources(doodle_core
        PUBLIC
        json_rpc_static_value.h

        PRIVATE
        json_rpc_static_value.cpp
        )
