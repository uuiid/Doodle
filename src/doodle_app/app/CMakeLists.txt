target_sources(doodle_app
        PUBLIC

        app_command.h
        program_options.h
        authorization.h

        PRIVATE
        app_command.cpp
        program_options.cpp
        authorization.cpp
        )
add_subdirectory(facet)
