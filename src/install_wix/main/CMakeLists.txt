set(CPACK_PACKAGE_VENDOR uuiid)
configure_file(cpack_variables.wxi cpack_variables.wxi)
configure_file(main.wxs main.wxs COPYONLY)
configure_file(${PROJECT_SOURCE_DIR}/LICENSE LICENSE.rtf COPYONLY)
#cmake_print_variables(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/cpack_variables.wxi
        ${CMAKE_CURRENT_BINARY_DIR}/main.wxs
        ${CMAKE_CURRENT_BINARY_DIR}/LICENSE.rtf
        DESTINATION wix)
find_package(Wix REQUIRED)

set(gen_wixobj_files)

macro(wix_heat_gen dir work_dir)
    add_custom_command(OUTPUT ${CMAKE_INSTALL_PREFIX}/wix/${dir}.wxs
            COMMAND py ${CMAKE_CURRENT_LIST_DIR}/../fix_main_wxs.py
            --input_dir ${CMAKE_INSTALL_PREFIX}/${dir}

            DEPENDS ${CMAKE_INSTALL_PREFIX}/${dir}
            WORKING_DIRECTORY ${work_dir}
            )
endmacro()

macro(wix_candle_gen file work_dir)
    add_custom_command(OUTPUT ${CMAKE_INSTALL_PREFIX}/wix/${file}.wixobj
            COMMAND wix_candle
            -nologo
            -arch x64
            -d${file}_dir=${CMAKE_INSTALL_PREFIX}/${file}
            -out "${work_dir}/wix/${file}.wixobj"
            "-I${work_dir}/wix"
            "-I${work_dir}"
            ${CMAKE_INSTALL_PREFIX}/wix/${file}.wxs

            DEPENDS ${CMAKE_INSTALL_PREFIX}/wix/${file}.wxs
            WORKING_DIRECTORY ${work_dir}
            )

    list(APPEND gen_wixobj_files ${CMAKE_INSTALL_PREFIX}/wix/${file}.wixobj)
endmacro(wix_candle_gen)

wix_heat_gen(bin ${CMAKE_INSTALL_PREFIX})


wix_heat_gen(maya ${CMAKE_INSTALL_PREFIX})
wix_heat_gen(ue425_Plug ${CMAKE_INSTALL_PREFIX})
wix_heat_gen(ue426_Plug ${CMAKE_INSTALL_PREFIX})
wix_heat_gen(ue427_Plug ${CMAKE_INSTALL_PREFIX})

wix_candle_gen(main ${CMAKE_INSTALL_PREFIX})
wix_candle_gen(bin ${CMAKE_INSTALL_PREFIX})
wix_candle_gen(maya ${CMAKE_INSTALL_PREFIX})
wix_candle_gen(ue425_Plug ${CMAKE_INSTALL_PREFIX})
wix_candle_gen(ue426_Plug ${CMAKE_INSTALL_PREFIX})
wix_candle_gen(ue427_Plug ${CMAKE_INSTALL_PREFIX})

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
        ${gen_wixobj_files}

        DEPENDS ${gen_wixobj_files}
        WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}
        )
