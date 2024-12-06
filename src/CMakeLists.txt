# set(FETCHCONTENT_FULLY_DISCONNECTED ON)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

include(ExternalProject)
include(FetchContent)


FetchContent_Declare(
        holidaycn
        GIT_REPOSITORY https://github.com/NateScarlet/holiday-cn.git
        GIT_TAG master
)

FetchContent_MakeAvailable(holidaycn)


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