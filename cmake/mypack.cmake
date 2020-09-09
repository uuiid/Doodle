function(CPACKIFW_COMMON)
    set(CPACK_PACKAGE_NAME Doodle)
    set(CPACK_PACKAGE_FILE_NAME installer)
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Installation Tool")
    set(CPACK_PACKAGE_VERSION "1.0.0") # Version of installer
    set(CPACK_IFW_PACKAGE_START_MENU_DIRECTORY Qt_CPackIFW)
    set(CPACK_GENERATOR IFW)
    set(CPACK_IFW_VERBOSE ON)

    include(CPack REQUIRED)
    include(CPackIFW REQUIRED)

    cpack_add_component(
        qt_cpackifw_installer
        DISPLAY_NAME "Qt CPackIFW"
        DESCRIPTION "Install me"
        REQUIRED
    )

    cpack_ifw_configure_component(
        qt_cpackifw_installer
        FORCED_INSTALLATION
        NAME qt.cpackifw.installer
        VERSION ${PROJECT_VERSION} # Version of component
        LICENSES License ${qt_cpackifw_SOURCE_DIR}/LICENSE
        DEFAULT TRUE
    )
endfunction()

function(dump_cmake_variables)
    get_cmake_property(_variableNames VARIABLES)
    list (SORT _variableNames)
    foreach (_variableName ${_variableNames})
        if (ARGV0)
            unset(MATCHED)
            string(REGEX MATCH ${ARGV0} MATCHED ${_variableName})
            if (NOT MATCHED)
                continue()
            endif()
        endif()
        message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()
endfunction()
