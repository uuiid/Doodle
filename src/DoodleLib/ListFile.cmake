include(CMakePrintHelpers)

function(doodle_get_source out_header out_source)
    set(options IS_EXE)
    set(oneValueArgs NAME)
    set(multiValueArgs LISTS_DIR)
    cmake_parse_arguments(
            DOODLE_GET_SOURCE
            "${options}"
            "${oneValueArgs}"
            "${multiValueArgs}"
            ${ARGN}
    )
    set(HEADER)
    set(SOURCE)
    foreach (D_DIR IN LISTS DOODLE_GET_SOURCE_LISTS_DIR)
        file(
                GLOB
                _HEADER
                LIST_DIRECTORIES false
                RELATIVE ${CMAKE_CURRENT_LIST_DIR}
                ${D_DIR}/*.h*
        )
        list(APPEND HEADER ${_HEADER})
        file(
                GLOB
                _SOURCE
                LIST_DIRECTORIES false
                RELATIVE ${CMAKE_CURRENT_LIST_DIR}
                ${D_DIR}/*.cpp
        )
        list(APPEND SOURCE ${_SOURCE})
    endforeach ()
    set("${out_source}" ${SOURCE} PARENT_SCOPE)
    set("${out_header}" ${HEADER} PARENT_SCOPE)
endfunction()

function(doodle_write_list_file)
    set(options IS_EXE)
    set(oneValueArgs NAME)
    set(multiValueArgs VAR_LIST)
    cmake_parse_arguments(
            DOODLE_WRITE_LIST_FILE
            "${options}"
            "${oneValueArgs}"
            "${multiValueArgs}"
            ${ARGN}
    )
    set(LIBCMAKE)
    foreach (VAR IN LISTS DOODLE_WRITE_LIST_FILE_VAR_LIST)
        string(APPEND LIBCMAKE "set(\n    ${VAR}\n    ")
        list(JOIN ${VAR} "\n    " LIBCMAKE_H)
        string(APPEND LIBCMAKE ${LIBCMAKE_H})
        string(APPEND LIBCMAKE ")\n")
    endforeach ()
    cmake_print_variables(LIBCMAKE)
endfunction()

doodle_get_source(
        DOODLELIB_HEADER
        DOODLELIB_SOURCE
        LISTS_DIR
        ${CMAKE_CURRENT_LIST_DIR}/core
        ${CMAKE_CURRENT_LIST_DIR}/Exception
        ${CMAKE_CURRENT_LIST_DIR}/FileSys
        ${CMAKE_CURRENT_LIST_DIR}/FileWarp
        ${CMAKE_CURRENT_LIST_DIR}/Gui
        ${CMAKE_CURRENT_LIST_DIR}/libWarp
        ${CMAKE_CURRENT_LIST_DIR}/Logger
        ${CMAKE_CURRENT_LIST_DIR}/main
        ${CMAKE_CURRENT_LIST_DIR}/Metadata
        ${CMAKE_CURRENT_LIST_DIR}/PinYin
        ${CMAKE_CURRENT_LIST_DIR}/rpc
        ${CMAKE_CURRENT_LIST_DIR}/ScreenshotWidght
        ${CMAKE_CURRENT_LIST_DIR}/threadPool
        ${CMAKE_CURRENT_LIST_DIR}/toolkit
)
doodle_get_source(
        DOODLE_GUI_HEADER
        DOODLE_GUI_SOURCE
        LISTS_DIR
        ${CMAKE_CURRENT_LIST_DIR}/Gui/Metadata
        ${CMAKE_CURRENT_LIST_DIR}/Gui/factory
        ${CMAKE_CURRENT_LIST_DIR}/Gui/action
        ${CMAKE_CURRENT_LIST_DIR}
)

doodle_write_list_file(
        VAR_LIST
        DOODLELIB_HEADER
        DOODLELIB_SOURCE
        DOODLE_GUI_HEADER
        DOODLE_GUI_SOURCE
)


# 连接头文件的字符串
set(LIBCMAKE "set(\n    DOODLELIB_HEADER\n    ")
list(JOIN DOODLELIB_HEADER "\n    " LIBCMAKE_H)
string(APPEND LIBCMAKE ${LIBCMAKE_H})
string(APPEND LIBCMAKE ")\n" "set(\n    " "DOODLELIB_SOURCE\n    ")

## 排除main目录中的文件
#list(FILTER DOODLELIB_SOURCE EXCLUDE REGEX "^main/")
## 排除预编译文件
#list(FILTER DOODLELIB_HEADER EXCLUDE REGEX ".*pch.h$")

# 连接源文件的字符串
list(JOIN DOODLELIB_SOURCE "\n    " LIBCMAKE_CPP)
string(APPEND LIBCMAKE ${LIBCMAKE_CPP})
string(APPEND LIBCMAKE "\n)")

#file(WRITE DoodleLib.cmake ${LIBCMAKE})
#
#file(WRITE DoodleLib.h [[
#//
#// Created by TD on 2021/5/9.
#//
#
##pragma once]])
#foreach (_HEADER IN LISTS DOODLELIB_HEADER)
#    if (${_HEADER} STREQUAL DoodleLib.h)
#    else ()
#        file(APPEND DoodleLib.h "\n#include <DoodleLib/${_HEADER}>")
#    endif ()
#endforeach ()
