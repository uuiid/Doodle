set(CPACK_PACKAGE_VENDOR uuiid)
configure_file(cpack_variables.wxi cpack_variables.wxi)
configure_file(main.wxs main.wxs COPYONLY)
#cmake_print_variables(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/cpack_variables.wxi
        ${CMAKE_CURRENT_BINARY_DIR}/main.wxs
        DESTINATION wix)
find_package(Wix REQUIRED)

add_custom_target(gen_heat_file

        COMMAND wix_heat
        dir ${CMAKE_INSTALL_PREFIX}/bin
        -dr DOODLE_ROOT
        -sreg
        -cg doodle_main_list
        -gg
        -out ${CMAKE_INSTALL_PREFIX}/wix/bin_files.wxs
        -var var.bin_dir

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

        WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}
        )
add_custom_target(gen_candle_file
        COMMAND wix_candle
        -nologo
        -arch x64
        -dbin_dir=${CMAKE_INSTALL_PREFIX}/bin
        -out "${CMAKE_INSTALL_PREFIX}/wix/bin_files.wixobj"
        "-I${CMAKE_INSTALL_PREFIX}/wix"
        "-I${CMAKE_INSTALL_PREFIX}/bin"
        ${CMAKE_INSTALL_PREFIX}/wix/bin_files.wxs

        COMMAND wix_candle
        -nologo
        -arch x64
        -out "${CMAKE_INSTALL_PREFIX}/wix/main.wixobj"
        "-I${CMAKE_INSTALL_PREFIX}/wix"
        "-I${CMAKE_INSTALL_PREFIX}"
        ${CMAKE_INSTALL_PREFIX}/wix/main.wxs

        DEPENDS gen_heat_file
        WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}
        )

include(ProcessorCount)
ProcessorCount(DOODLE_CPU_N)
add_custom_target(gen_light_file
        COMMAND wix_light
        -nologo
        -out "${CMAKE_INSTALL_PREFIX}/Doodle-test.msi"
        -ext "WixUIExtension"
        -cultures:zh-CN
        -ct ${DOODLE_CPU_N}
#        -b ${CMAKE_INSTALL_PREFIX}/bin
        "${CMAKE_INSTALL_PREFIX}/wix/main.wixobj"
        "${CMAKE_INSTALL_PREFIX}/wix/bin_files.wixobj"

        DEPENDS gen_candle_file
        WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}
        )
