configure_file(main.wxs limited_main.wxs COPYONLY)


install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/limited_main.wxs
        DESTINATION wix)


set(gen_wixobj_limited_files)

wix_candle_gen(main_limited ${CMAKE_INSTALL_PREFIX} gen_wixobj_limited_files)


include(ProcessorCount)
ProcessorCount(DOODLE_CPU_N)

add_custom_target(gen_light_file_limited
        COMMAND wix_light
        -nologo
        -out "${CMAKE_INSTALL_PREFIX}/${DOODLE_PACKAGE_NAME}_limited.msi"
        -ext "WixUIExtension"
        -cultures:zh-CN
        -ct ${DOODLE_CPU_N}
        #        -b ${CMAKE_INSTALL_PREFIX}/bin
        -spdb # 取消pdb文件输出
        ${gen_wixobj_limited_files}
        ${CMAKE_INSTALL_PREFIX}/wix/maya.wixobj
        ${CMAKE_INSTALL_PREFIX}/wix/bin.wixobj


        COMMAND 7z a -mx2 -mmt8
        "${CMAKE_INSTALL_PREFIX}/${DOODLE_PACKAGE_NAME}_limited.7z"
        "${CMAKE_INSTALL_PREFIX}/bin_limited"
        "${CMAKE_INSTALL_PREFIX}/maya"

        DEPENDS ${gen_wixobj_limited_files}
        WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}


        )
