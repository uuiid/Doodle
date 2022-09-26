

set(FETCHCONTENT_FULLY_DISCONNECTED ON)


include(ExternalProject)
include(FetchContent)

FetchContent_Declare(
        CMakeRCLib
        GIT_REPOSITORY https://github.com/vector-of-bool/cmrc.git
        GIT_TAG master
)


FetchContent_Declare(
        TimeLib
        URL https://www.iana.org/time-zones/repository/tzdata-latest.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

message("TimeLib_SOURCE_DIR = ${timelib_SOURCE_DIR}")



FetchContent_MakeAvailable(CMakeRCLib TimeLib)

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
