target_sources(doodle_app
        PUBLIC

        app_command.h
        program_options.h
        short_cut.h

        PRIVATE

        app_command.cpp
        program_options.cpp
        short_cut.cpp
        )
add_subdirectory(facet)
