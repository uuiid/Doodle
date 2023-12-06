target_sources(doodle_app
        PUBLIC

        app_command.h
        program_options.h
        authorization.h
        gui_log.h

        PRIVATE
        app_command.cpp
        program_options.cpp
        authorization.cpp
        gui_log.cpp
        )
add_subdirectory(facet)
