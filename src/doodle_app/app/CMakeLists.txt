target_sources(doodle_app
        PUBLIC

        app_command.h
        program_options.h
        short_cut.h
        this_rpc_exe.h
        authorization.h

        PRIVATE
        app_command.cpp
        program_options.cpp
        short_cut.cpp
        this_rpc_exe.cpp
        authorization.cpp
        )
add_subdirectory(facet)
