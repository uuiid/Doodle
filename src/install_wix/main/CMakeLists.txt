set(CPACK_PACKAGE_VENDOR uuiid)
configure_file(cpack_variables.wxi cpack_variables.wxi)
#cmake_print_variables(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cpack_variables.wxi
        DESTINATION wix)
