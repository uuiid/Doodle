include(CMakePrintHelpers)
include(${DOODLE_INCLUDE_CMAKE}/CMake/doodle_fun_tool.cmake)

doodle_get_source(
        DOODLELIB_HEADER
        DOODLELIB_SOURCE
        LISTS_DIR
        ${CMAKE_CURRENT_LIST_DIR}/core
        ${CMAKE_CURRENT_LIST_DIR}/Exception
        ${CMAKE_CURRENT_LIST_DIR}/FileSys
        ${CMAKE_CURRENT_LIST_DIR}/FileWarp
        ${CMAKE_CURRENT_LIST_DIR}/Gui
        ${CMAKE_CURRENT_LIST_DIR}/Gui/factory
        ${CMAKE_CURRENT_LIST_DIR}/Gui/action
        ${CMAKE_CURRENT_LIST_DIR}/Gui/widgets
        ${CMAKE_CURRENT_LIST_DIR}/libWarp
        ${CMAKE_CURRENT_LIST_DIR}/Logger
        ${CMAKE_CURRENT_LIST_DIR}/Metadata
        ${CMAKE_CURRENT_LIST_DIR}/PinYin
        ${CMAKE_CURRENT_LIST_DIR}/rpc
        ${CMAKE_CURRENT_LIST_DIR}/ScreenshotWidght
        ${CMAKE_CURRENT_LIST_DIR}/threadPool
        ${CMAKE_CURRENT_LIST_DIR}/toolkit
        ${CMAKE_CURRENT_LIST_DIR}
)

doodle_write_list_file(
        VAR_LIST
        DOODLELIB_HEADER
        DOODLELIB_SOURCE
        NAME
        DoodleLib
)

file(WRITE doodle_lib_all.h [[//
// Created by TD on 2021/5/9.
//

#pragma once]] )
foreach (_HEADER IN LISTS DOODLELIB_HEADER)
    if (${_HEADER} STREQUAL doodle_lib_all.h)
    else()
        file(APPEND doodle_lib_all.h "\n#include <DoodleLib/${_HEADER}>")
    endif ()
endforeach ()
