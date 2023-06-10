# set(FETCHCONTENT_FULLY_DISCONNECTED ON)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

include(ExternalProject)
include(FetchContent)

# FetchContent_Declare(
# TimeLib
# URL https://www.iana.org/time-zones/repository/tzdata-latest.tar.gz
# DOWNLOAD_EXTRACT_TIMESTAMP TRUE
# )
FetchContent_Declare(
        InnoLagnuage
        URL https://raw.github.com/jrsoftware/issrc/main/Files/Languages/Unofficial/ChineseSimplified.isl
        DOWNLOAD_NO_EXTRACT TRUE
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

FetchContent_MakeAvailable(fontawesome IconFontCppHeaders doxygenAwesomeCss holidaycn innolagnuage)

# =====================文档过程===============================
# 添加文档生成
if (CMAKE_BUILD_TYPE EQUAL "DEBUG")
    set(DOXYGEN_WARNINGS YES)
else ()
    set(DOXYGEN_WARNINGS NO)
endif ()
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

add_subdirectory(exe_gui)
add_subdirectory(houdini_plug)

if (DEFINED Maya_Version)
    # 寻找 maya 包(包括各种动态库) 会在 ${CMAKE_PREFIX_PATH} 这里找 -> 找到 `FindMaya.cmake`
    # 模块的寻找文件名称 Find + 包名称 + .cmake
    # 开始运行 `FindMaya.cmake`
    find_package(Maya ${Maya_Version} MODULE)

    if (TARGET maya::maya_all)
        find_package(OpenMP REQUIRED)
        find_package(Alembic CONFIG REQUIRED)
        find_package(dem-bones CONFIG REQUIRED)
        add_subdirectory(maya_plug_mll maya_plug_mll${Maya_Version})

        add_subdirectory(maya_plug maya_plug${Maya_Version})
        add_subdirectory(exe_maya exe_maya${Maya_Version})
    endif ()
endif ()

add_subdirectory(photoshop_plug)
add_subdirectory(exe_authorization)

find_package(Inno)

if (TARGET Inno_exe)
    add_subdirectory(install)
endif ()

add_subdirectory(test)
