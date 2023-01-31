
macro(wix_heat_gen dir work_dir)
        add_custom_command(OUTPUT ${CMAKE_INSTALL_PREFIX}/wix/${dir}.wxs
                COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INSTALL_PREFIX}/${dir}
                COMMAND py ${CMAKE_CURRENT_LIST_DIR}/../fix_main_wxs.py
                --input_dir ${CMAKE_INSTALL_PREFIX}/${dir}

                DEPENDS ${CMAKE_INSTALL_PREFIX}/${dir}
                WORKING_DIRECTORY ${work_dir}
        )
endmacro()

macro(wix_candle_gen file work_dir gen_wixobj_files)
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

        list(APPEND ${gen_wixobj_files} ${CMAKE_INSTALL_PREFIX}/wix/${file}.wixobj)
endmacro(wix_candle_gen)


#add_subdirectory(limited_fbx)
#add_subdirectory(main)
find_package(Pandoc)

set(DOODLE_PACKAGE_VENDOR uuiid)
configure_file(cpack_variables.wxi cpack_variables.wxi)

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/cpack_variables.wxi
        main.wxs
        DESTINATION wix
        COMPONENT exe_com)


install(CODE "
execute_process(
        COMMAND \"$<TARGET_FILE:pandoc>\" -s ${PROJECT_SOURCE_DIR}/LICENSE -f textile -o ./wix/LICENSE.rtf
        WORKING_DIRECTORY \${CMAKE_INSTALL_PREFIX}
)
"
        COMPONENT exe_com
        )