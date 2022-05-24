set(CPACK_PACKAGE_VENDOR uuiid)
configure_file(cpack_variables.wxi cpack_variables.wxi)
#cmake_print_variables(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cpack_variables.wxi
        DESTINATION wix)
find_package(Wix REQUIRED)

add_custom_target(get_main_exe_file_list

        COMMAND wix_heat
        dir ${CMAKE_INSTALL_PREFIX}/bin
        -srd
        -dr doodle_root_bin
        -sreg
        -cg doodle_main_list
        -out ${CMAKE_INSTALL_PREFIX}/wix/bin_files.wxs
        -var var.doodle_bin

        COMMAND wix_heat
        dir ${CMAKE_INSTALL_PREFIX}/maya
        -srd
        -dr doodle_root_maya
        -sreg
        -cg doodle_maya_list
        -out ${CMAKE_INSTALL_PREFIX}/wix/maya_files.wxs
        -var var.doodle_maya

        COMMAND wix_heat
        dir ${CMAKE_INSTALL_PREFIX}/ue4.25_Plug
        -srd
        -dr doodle_root_ue425
        -sreg
        -cg doodle_ue425_list
        -out ${CMAKE_INSTALL_PREFIX}/wix/ue425.wxs
        -var var.doodle_ue425

        COMMAND wix_heat
        dir ${CMAKE_INSTALL_PREFIX}/ue4.26_Plug
        -srd
        -dr doodle_root_ue426
        -sreg
        -cg doodle_ue426_list
        -out ${CMAKE_INSTALL_PREFIX}/wix/ue426.wxs
        -var var.doodle_ue426

        COMMAND wix_heat
        dir ${CMAKE_INSTALL_PREFIX}/ue4.27_Plug
        -srd
        -dr doodle_root_ue427
        -sreg
        -cg doodle_ue427_list
        -out ${CMAKE_INSTALL_PREFIX}/wix/ue427.wxs
        -var var.doodle_ue427
        )
