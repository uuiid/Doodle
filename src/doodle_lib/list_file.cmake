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
        ${CMAKE_CURRENT_LIST_DIR}/metadata
        ${CMAKE_CURRENT_LIST_DIR}/pin_yin
        ${CMAKE_CURRENT_LIST_DIR}/rpc
        ${CMAKE_CURRENT_LIST_DIR}/screenshot_widght
        ${CMAKE_CURRENT_LIST_DIR}/thread_pool
        ${CMAKE_CURRENT_LIST_DIR}/toolkit
        ${CMAKE_CURRENT_LIST_DIR}
)

doodle_write_list_file(
        VAR_LIST
        DOODLELIB_HEADER
        DOODLELIB_SOURCE
        NAME
        doodle_lib
)

file(WRITE doodle_lib_all.h [[//
// Created by TD on 2021/5/9.
//

#pragma once]] )
foreach (_HEADER IN LISTS DOODLELIB_HEADER)
    if (${_HEADER} STREQUAL doodle_lib_all.h)
    else()
        file(APPEND doodle_lib_all.h "\n#include <doodle_lib/${_HEADER}>")
    endif ()
endforeach ()
