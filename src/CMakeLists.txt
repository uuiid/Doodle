

set(FETCHCONTENT_FULLY_DISCONNECTED ON)


include(ExternalProject)
include(FetchContent)

FetchContent_Declare(
        CMakeRCLib
        GIT_REPOSITORY https://github.com/vector-of-bool/cmrc.git
        GIT_TAG master
)
FetchContent_MakeAvailable(CMakeRCLib)

message("CMakeRCLib_SOURCE_DIR = ${cmakerclib_SOURCE_DIR}")

list(APPEND CMAKE_PREFIX_PATH ${cmakerclib_SOURCE_DIR})
include(CMakeRC)


add_subdirectory(doodle_core)
add_subdirectory(doodle_dingding)
add_subdirectory(doodle_lib)
add_subdirectory(doodle_exe)
add_subdirectory(doodle_tool_lib)
add_subdirectory(houdini_plug)
add_subdirectory(maya_plug)
add_subdirectory(install_wix)
add_subdirectory(test)
