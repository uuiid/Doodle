include(${CMAKE_CURRENT_LIST_DIR}/Ninja_release/CPackConfig.cmake)

set(CPACK_INSTALL_CMAKE_PROJECTS
        "${CMAKE_CURRENT_LIST_DIR}/Ninja_release;Doodle;main_exe;/"
#        "Ninja_release;Doodle;Doxygen;/"
        )
