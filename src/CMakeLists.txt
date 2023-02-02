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
        doxygenAwesomeCss
        GIT_REPOSITORY https://github.com/jothepro/doxygen-awesome-css.git
        GIT_TAG main
)

FetchContent_Declare(
        IconFontCppHeaders
        GIT_REPOSITORY https://github.com/juliettef/IconFontCppHeaders.git
        GIT_TAG main
)

FetchContent_Declare(
        holidaycn
        GIT_REPOSITORY https://github.com/NateScarlet/holiday-cn.git
        GIT_TAG master
)

FetchContent_MakeAvailable(CMakeRCLib TimeLib fontawesome IconFontCppHeaders doxygenAwesomeCss holidaycn)

#=====================文档过程===============================
#添加文档生成
set(DOXYGEN_WARNINGS YES)
set(DOXYGEN_WARN_IF_UNDOCUMENTED YES)
set(DOXYGEN_WARN_IF_DOC_ERROR YES)
set(DOXYGEN_OUTPUT_LANGUAGE Chinese)
set(DOXYGEN_EXCLUDE
        doodle_lib/external
        doodle_lib/resource
        doodle_core/generate
        doodle_lib/rpc
        )
set(DOXYGEN_BUILTIN_STL_SUPPORT YES)
# set(DOXYGEN_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build)

# set(DOXYGEN_HTML_INDEX_NUM_ENTRIES 0)
# set(DOXYGEN_HTML_DYNAMIC_SECTIONS YES)
set(DOXYGEN_QUIET YES)
set(DOXYGEN_PROJECT_LOGO doodle_exe/main/icon.ico)
set(DOXYGEN_NUM_PROC_THREADS 0)

# 创建 css 必须的一部份设置
set(DOXYGEN_GENERATE_TREEVIEW YES)
set(DOXYGEN_DISABLE_INDEX NO)
set(DOXYGEN_FULL_SIDEBAR NO)
set(DOXYGEN_HTML_EXTRA_STYLESHEET ${doxygenawesomecss_SOURCE_DIR}/doxygen-awesome.css)

doxygen_add_docs(Doxygen
        doodle_core
        doodle_app
        doodle_lib
        doodle_dingding
        maya_plug
        )

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
add_subdirectory(doodle_lib)
if (BUILD_LIMITED_1)
    message("build task_exe BUILD_LIMITED_1=${BUILD_LIMITED_1}")
    add_subdirectory(exe_task)
else ()
    message("build GUI BUILD_LIMITED_1=${BUILD_LIMITED_1}")
    add_subdirectory(exe_gui)
endif ()

add_subdirectory(houdini_plug)
if (DEFINED Maya_Version)
    add_subdirectory(maya_plug maya_plug${Maya_Version})
endif ()
add_subdirectory(photoshop_plug)
add_subdirectory(exe_authorization)

find_package(Wix REQUIRED)
if (TARGET wix_exe)
    add_subdirectory(install)
endif ()

add_subdirectory(test)
