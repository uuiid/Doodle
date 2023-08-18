target_sources(
        doodle_lib
        PUBLIC
        client.h
        working_machine.h
        working_machine_session.h

        PRIVATE
        client.cpp
        working_machine.cpp
        working_machine_session.cpp
)
add_subdirectory(exception)
add_subdirectory(detail)
