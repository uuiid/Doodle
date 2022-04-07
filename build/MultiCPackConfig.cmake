include(${CMAKE_CURRENT_LIST_DIR}/Ninja_release/CPackConfig.cmake)

set(CPACK_INSTALL_CMAKE_PROJECTS
        "${CMAKE_CURRENT_LIST_DIR}/Ninja_release;Doodle;main_exe;/"
        "${CMAKE_CURRENT_LIST_DIR}/Ninja_release_maya_2018;Doodle;maya_plug;/"
        "${CMAKE_CURRENT_LIST_DIR}/Ninja_release_maya_2019;Doodle;maya_plug;/"
        "${CMAKE_CURRENT_LIST_DIR}/Ninja_release_maya_2020;Doodle;maya_plug;/"
        "${CMAKE_CURRENT_LIST_DIR}/ue4_release_27;Doodle;ue4_plug;/"
        )
