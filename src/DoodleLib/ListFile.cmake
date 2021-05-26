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
set(LIBCMAKE "set(\n\tDOODLELIB_HEADER\n\t")
list(JOIN DOODLELIB_HEADER "\n\t" LIBCMAKE_H)
string(APPEND LIBCMAKE ${LIBCMAKE_H})
string(APPEND LIBCMAKE ")\n" "set(\n\t" "DOODLELIB_SOURCE\n\t")
list(JOIN DOODLELIB_SOURCE "\n\t" LIBCMAKE_CPP)
string(APPEND LIBCMAKE ${LIBCMAKE_CPP})
string(APPEND LIBCMAKE "\n)")

file(WRITE DoodleLib.cmake ${LIBCMAKE})

file(WRITE DoodleLib.h [[//
// Created by TD on 2021/5/9.
//

#pragma once]] )
foreach (_HEADER IN LISTS DOODLELIB_HEADER)
    if (${_HEADER} STREQUAL DoodleLib.h)
    else()
        file(APPEND DoodleLib.h "\n#include <DoodleLib/${_HEADER}>")
    endif ()
endforeach ()
