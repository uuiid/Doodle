find_package(Pandoc)

set(DOODLE_PACKAGE_VENDOR uuiid)
configure_file(cpack_variables.wxi cpack_variables.wxi)

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/cpack_variables.wxi
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

include(ProcessorCount)
ProcessorCount(DOODLE_CPU_N)
find_package(SevenZip)

configure_file(doodle_main_install.cmake doodle_main_install.cmake @ONLY)
install(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/doodle_main_install.cmake
        COMPONENT exe_com
        )
