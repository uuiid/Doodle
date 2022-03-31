include(CMakePrintHelpers)

function(get_git_version)
    set(options IS)
    set(oneValueArgs WORK_DIR)
    set(multiValueArgs DIR)
    cmake_parse_arguments(
            DOODLE_GIT
            "${options}"
            "${oneValueArgs}"
            "${multiValueArgs}"
            ${ARGN}
    )

    find_package(Git)
#    if(Git_FOUND)
#        message("Git found: ${GIT_EXECUTABLE}")
#    endif()
    execute_process( # get_git_version
            COMMAND ${GIT_EXECUTABLE} describe --tags --dirty --match "v*"
            WORKING_DIRECTORY ${DOODLE_GIT_WORK_DIR}
            OUTPUT_VARIABLE GIT_DESCRIBE_VERSION
            RESULT_VARIABLE GIT_DESCRIBE_ERROR_CODE
            OUTPUT_STRIP_TRAILING_WHITESPACE)

#    cmake_print_variables(
#            DOODLE_GIT_WORK_DIR
#            GIT_DESCRIBE_ERROR_CODE
#            GIT_DESCRIBE_VERSION)

    if (NOT GIT_DESCRIBE_ERROR_CODE)
        string(REGEX REPLACE "^v([0-9]+)\\..*" "\\1" VERSION_MAJOR "${GIT_DESCRIBE_VERSION}")
        string(REGEX REPLACE "^v[0-9]+\\.([0-9]+).*" "\\1" VERSION_MINOR "${GIT_DESCRIBE_VERSION}")
        string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" VERSION_PATCH "${GIT_DESCRIBE_VERSION}")
        string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.[0-9]+-([0-9]+).*" "\\1" VERSION_TWEAK "${GIT_DESCRIBE_VERSION}")
        string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.[0-9]+-[0-9]+-(.*)" "\\1" VERSION_SHA1 "${GIT_DESCRIBE_VERSION}")
        set(VERSION_SHORT "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}")
#        cmake_print_variables(
#                VERSION_MAJOR
#                VERSION_MINOR
#                VERSION_PATCH
#                VERSION_SHA1
#                VERSION_SHORT
#                VERSION_TWEAK)
        set(v_major ${VERSION_MAJOR} PARENT_SCOPE)
        set(v_minor ${VERSION_MINOR} PARENT_SCOPE)
        set(v_patch ${VERSION_PATCH} PARENT_SCOPE)
        set(v_tweak ${VERSION_TWEAK} PARENT_SCOPE)
        set(v_sha1 ${VERSION_SHA1} PARENT_SCOPE)
        set(v_short ${VERSION_SHORT} PARENT_SCOPE)

    endif ()
endfunction()
