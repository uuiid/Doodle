add_executable(generate_token
        generate_token.cpp
        )


target_link_libraries(generate_token
        PUBLIC
        doodle_lib
        )