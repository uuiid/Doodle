# set(FETCHCONTENT_FULLY_DISCONNECTED ON)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

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

FetchContent_Declare(
        fontawesome
        GIT_REPOSITORY https://github.com/FortAwesome/Font-Awesome.git
        GIT_TAG 6.x
)

FetchContent_Declare(
        IconFontCppHeaders
        GIT_REPOSITORY https://github.com/juliettef/IconFontCppHeaders.git
        GIT_TAG main
)

FetchContent_MakeAvailable(CMakeRCLib TimeLib fontawesome IconFontCppHeaders)

list(APPEND CMAKE_MODULE_PATH ${cmakerclib_SOURCE_DIR})
include(CMakeRC)

cmrc_add_resource_library(
        DoodleLibResourceFont
        WHENCE ${fontawesome_SOURCE_DIR}/webfonts
        ${fontawesome_SOURCE_DIR}/webfonts/fa-brands-400.ttf
        ${fontawesome_SOURCE_DIR}/webfonts/fa-regular-400.ttf
        ${fontawesome_SOURCE_DIR}/webfonts/fa-solid-900.ttf
)
add_subdirectory(doodle_app)
add_subdirectory(doodle_core)
add_subdirectory(doodle_dingding)
add_subdirectory(doodle_lib)
add_subdirectory(doodle_exe)
add_subdirectory(houdini_plug)
add_subdirectory(maya_plug)
add_subdirectory(install_wix)
add_subdirectory(test)
