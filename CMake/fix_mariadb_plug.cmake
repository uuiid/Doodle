include(CMakePrintHelpers)


function(FIX_MARIADB_PLUG)
    set(options IS_)
    set(oneValueArgs NAME)
    set(multiValueArgs LISTS_FILES)
    cmake_parse_arguments(
            FIX_MARIADB_PLUG
            "${options}"
            "${oneValueArgs}"
            "${multiValueArgs}"
            ${ARGN}
    )
    set(LIBMARIADB_PLUG
            auth_gssapi_client
            caching_sha2_password
            client_ed25519
            dialog
            mysql_clear_password
            pvio_npipe
            pvio_shmem
            sha256_password)
    get_target_property(_DIR ${FIX_MARIADB_PLUG_NAME} RUNTIME_OUTPUT_DIRECTORY)
    if (NOT _DIR)
        set(_DIR ${CMAKE_CURRENT_BINARY_DIR})
    endif ()


#     cmake_print_variables(FIX_MARIADB_PLUG_NAME _DIR)
    foreach (VAR IN LISTS LIBMARIADB_PLUG )
        # cmake_print_variables(VAR)
#        set(CMAKE_FIND_DEBUG_MODE TRUE)
        find_file(
                _PLUG_${VAR}
                NAMES ${VAR}.dll
                HINTS /plugins/libmariadb/
                REQUIRED
        )
#        set(CMAKE_FIND_DEBUG_MODE FALSE)
        # cmake_print_variables(_PLUG_${VAR})
        add_custom_command(
                TARGET ${FIX_MARIADB_PLUG_NAME}
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_PLUG_${VAR}} ${_DIR}
        )
    endforeach ()

endfunction()
