target_sources(
        doodle_lib
        PUBLIC
        working_machine.h
        working_machine_session.h

        PRIVATE
        working_machine.cpp
        working_machine_session.cpp
)
add_subdirectory(exception)
add_subdirectory(detail)
