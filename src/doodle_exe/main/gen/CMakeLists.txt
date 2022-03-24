add_executable(generate_token
        generate_token.cpp
        ../DoodleExe.manifest
        ../win_exe.rc
        ../icon.ico
        )


target_link_libraries(generate_token
        PUBLIC
        doodle_lib
        )

target_compile_definitions(generate_token
        PUBLIC
        ISOLATION_AWARE_ENABLED
        )

add_custom_target(
        generate_token_run
        POST_BUILD
        COMMAND $<TARGET_FILE:generate_token> ${CMAKE_CURRENT_BINARY_DIR}/bin/
        DEPENDS generate_token
)
