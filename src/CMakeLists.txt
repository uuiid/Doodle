# set(FETCHCONTENT_FULLY_DISCONNECTED ON)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

include(ExternalProject)
include(FetchContent)

FetchContent_Declare(
        doxygenAwesomeCss
        GIT_REPOSITORY https://github.com/jothepro/doxygen-awesome-css.git
        GIT_TAG main
)

FetchContent_Declare(
        holidaycn
        GIT_REPOSITORY https://github.com/NateScarlet/holiday-cn.git
        GIT_TAG master
)

FetchContent_MakeAvailable(doxygenAwesomeCss holidaycn)

# =====================文档过程===============================
# 添加文档生成
if (CMAKE_BUILD_TYPE EQUAL "DEBUG")
    set(DOXYGEN_WARNINGS YES)
else ()
    set(DOXYGEN_EXTRACT_ALL YES)
    set(DOXYGEN_WARNINGS NO)
    set(DOXYGEN_QUIET YES)
endif ()
set(DOXYGEN_WARN_IF_UNDOCUMENTED YES)
set(DOXYGEN_WARN_IF_DOC_ERROR YES)
set(DOXYGEN_OUTPUT_LANGUAGE Chinese)
set(DOXYGEN_BUILTIN_STL_SUPPORT YES)

# set(DOXYGEN_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build)

# set(DOXYGEN_HTML_INDEX_NUM_ENTRIES 0)
# set(DOXYGEN_HTML_DYNAMIC_SECTIONS YES)
set(DOXYGEN_NUM_PROC_THREADS 0)

# 创建 css 必须的一部份设置
set(DOXYGEN_GENERATE_TREEVIEW YES)
set(DOXYGEN_DISABLE_INDEX NO)
set(DOXYGEN_FULL_SIDEBAR NO)
set(DOXYGEN_HTML_EXTRA_STYLESHEET ${doxygenawesomecss_SOURCE_DIR}/doxygen-awesome.css)

doxygen_add_docs(Doxygen
        doodle_core
        doodle_lib
        maya_plug
        exe_maya
        exe_gui
        maya_plug_mll
)

add_subdirectory(doodle_core)
add_subdirectory(doodle_lib)

add_subdirectory(exe_gui)

if (DEFINED Maya_Version)
    # 寻找 maya 包(包括各种动态库) 会在 ${CMAKE_PREFIX_PATH} 这里找 -> 找到 `FindMaya.cmake`
    # 模块的寻找文件名称 Find + 包名称 + .cmake
    # 开始运行 `FindMaya.cmake`
    find_package(Maya ${Maya_Version} MODULE)

    if (TARGET maya::maya_all)
        set(MAYA_LOCATION ${MAYA_BASE_DIR})
        set(IS_WINDOWS TRUE)
        find_package(Maya_Qt MODULE COMPONENTS Core Gui Widgets REQUIRED)

        find_package(OpenMP REQUIRED)
        find_package(Alembic CONFIG REQUIRED)
        find_package(dem-bones CONFIG REQUIRED)
        add_subdirectory(maya_plug_mll maya_plug_mll${Maya_Version})

        add_subdirectory(maya_plug maya_plug${Maya_Version})
        add_subdirectory(exe_maya exe_maya${Maya_Version})
    endif ()
endif ()

add_subdirectory(exe_authorization)
add_subdirectory(test)
add_subdirectory(usd)