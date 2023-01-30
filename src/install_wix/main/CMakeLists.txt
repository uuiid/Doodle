# 寻找 pandoc
find_package(Pandoc)

set(DOODLE_PACKAGE_VENDOR uuiid)
configure_file(cpack_variables.wxi cpack_variables.wxi)
configure_file(main.wxs main.wxs COPYONLY)

configure_file(${PROJECT_SOURCE_DIR}/LICENSE LICENSE.rtf COPYONLY)
#cmake_print_variables(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/cpack_variables.wxi
        ${CMAKE_CURRENT_BINARY_DIR}/main.wxs
        ${CMAKE_CURRENT_BINARY_DIR}/LICENSE.rtf
        DESTINATION wix
        COMPONENT exe_com)

set(gen_wixobj_files)

wix_heat_gen(bin ${CMAKE_INSTALL_PREFIX})


wix_heat_gen(maya ${CMAKE_INSTALL_PREFIX})
wix_heat_gen(ue427_Plug ${CMAKE_INSTALL_PREFIX})
wix_heat_gen(ue50_Plug ${CMAKE_INSTALL_PREFIX})
wix_heat_gen(ue51_Plug ${CMAKE_INSTALL_PREFIX})
wix_heat_gen(houdini ${CMAKE_INSTALL_PREFIX})
wix_heat_gen(SideFX_Labs ${CMAKE_INSTALL_PREFIX})

wix_candle_gen(main ${CMAKE_INSTALL_PREFIX} gen_wixobj_files)
wix_candle_gen(bin ${CMAKE_INSTALL_PREFIX} gen_wixobj_files)
wix_candle_gen(maya ${CMAKE_INSTALL_PREFIX} gen_wixobj_files)
wix_candle_gen(ue427_Plug ${CMAKE_INSTALL_PREFIX} gen_wixobj_files)
wix_candle_gen(ue50_Plug ${CMAKE_INSTALL_PREFIX} gen_wixobj_files)
wix_candle_gen(ue51_Plug ${CMAKE_INSTALL_PREFIX} gen_wixobj_files)
wix_candle_gen(houdini ${CMAKE_INSTALL_PREFIX} gen_wixobj_files)
wix_candle_gen(SideFX_Labs ${CMAKE_INSTALL_PREFIX} gen_wixobj_files)

include(ProcessorCount)
ProcessorCount(DOODLE_CPU_N)
find_package(SevenZip)

add_custom_target(gen_light_file
        COMMAND wix_light
        -nologo
        -out "${CMAKE_INSTALL_PREFIX}/${DOODLE_PACKAGE_NAME}.msi"
        -ext "WixUIExtension"
        -cultures:zh-CN
        -ct ${DOODLE_CPU_N}
        -cc "${CMAKE_INSTALL_PREFIX}"
        -reusecab

        # -b ${CMAKE_INSTALL_PREFIX}/bin
        -spdb # 取消pdb文件输出
        ${gen_wixobj_files}

        COMMAND ${SEVENZIP_BIN} a -mx2 -mmt8
        "${CMAKE_INSTALL_PREFIX}/${DOODLE_PACKAGE_NAME}.7z"
        "${CMAKE_INSTALL_PREFIX}/bin"
        "${CMAKE_INSTALL_PREFIX}/maya"
        "${CMAKE_INSTALL_PREFIX}/ue427_Plug"
        "${CMAKE_INSTALL_PREFIX}/ue50_Plug"
        "${CMAKE_INSTALL_PREFIX}/ue51_Plug"
        "${CMAKE_INSTALL_PREFIX}/houdini"
        "${CMAKE_INSTALL_PREFIX}/SideFX_Labs"
        -x!bin/doodle_limited_exe.exe

        DEPENDS ${gen_wixobj_files}
        WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}
)
