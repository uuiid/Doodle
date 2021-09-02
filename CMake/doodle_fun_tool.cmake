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
    file(WRITE ${DOODLE_WRITE_LIST_FILE_NAME}.cmake ${LIBCMAKE})
endfunction()
