include(CMakePrintHelpers)

file(
        GLOB_RECURSE
        DOODLELIB_HEADER
        LIST_DIRECTORIES false
        RELATIVE ${CMAKE_CURRENT_LIST_DIR}
        *.h*
)
file(
        GLOB_RECURSE
        DOODLELIB_SOURCE
        LIST_DIRECTORIES false
        RELATIVE ${CMAKE_CURRENT_LIST_DIR}
        *.cpp
)
# 排除resource目录中的文件
list(FILTER DOODLELIB_HEADER EXCLUDE REGEX "^resource/")

# 连接头文件的字符串
set(LIBCMAKE "set(\n    DOODLELIB_HEADER\n    ")
list(JOIN DOODLELIB_HEADER "\n    " LIBCMAKE_H)
string(APPEND LIBCMAKE ${LIBCMAKE_H})
string(APPEND LIBCMAKE ")\n" "set(\n    " "DOODLELIB_SOURCE\n    ")

# 排除main目录中的文件
list(FILTER DOODLELIB_SOURCE EXCLUDE REGEX "^main/")
# 排除预编译文件
list(FILTER DOODLELIB_HEADER EXCLUDE REGEX ".*pch.h$")

# 连接源文件的字符串
list(JOIN DOODLELIB_SOURCE "\n    " LIBCMAKE_CPP)
string(APPEND LIBCMAKE ${LIBCMAKE_CPP})
string(APPEND LIBCMAKE "\n)")

cmake_print_variables(
        DOODLELIB_HEADER
        DOODLELIB_SOURCE
        LIBCMAKE)
file(WRITE DoodleLib.cmake ${LIBCMAKE})

file(WRITE DoodleLib.h [[
//
// Created by TD on 2021/5/9.
//

#pragma once]])
foreach (_HEADER IN LISTS DOODLELIB_HEADER)
    if (${_HEADER} STREQUAL DoodleLib.h)
    else ()
        file(APPEND DoodleLib.h "\n#include <DoodleLib/${_HEADER}>")
    endif ()
endforeach ()
