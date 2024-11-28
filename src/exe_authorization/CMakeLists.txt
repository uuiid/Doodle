add_executable(generate_token
        generate_token.cpp
        )


target_link_libraries(generate_token
        PUBLIC
        doodle_lib
        doodle_core
        )

install(
        CODE
        "
        execute_process(COMMAND $<TARGET_FILE:generate_token>
        WORKING_DIRECTORY \${CMAKE_INSTALL_PREFIX}/bin
        )
        "
)