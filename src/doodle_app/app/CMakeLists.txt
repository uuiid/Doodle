target_sources(doodle_app
        PUBLIC

        app_command.h
        program_options.h
        this_rpc_exe.h
        authorization.h

        PRIVATE
        app_command.cpp
        program_options.cpp
        this_rpc_exe.cpp
        authorization.cpp
        )
add_subdirectory(facet)
