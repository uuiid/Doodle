add_executable(doodleServer
        WIN32
        doodle_server.manifest
        win_server_exe.rc
        main.cpp
        )
set_target_properties(doodleServer 
PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

target_link_libraries(
        doodleServer
        PUBLIC
        DoodleLib
)

