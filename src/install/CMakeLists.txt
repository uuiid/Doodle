find_package(Pandoc)

set(DOODLE_PACKAGE_VENDOR uuiid)
configure_file(cpack_variables.wxi cpack_variables.wxi)

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/cpack_variables.wxi
        DESTINATION wix
        COMPONENT exe_com)


include(ProcessorCount)
ProcessorCount(DOODLE_CPU_N)
find_package(SevenZip)

configure_file(doodle_main_install.cmake doodle_main_install.cmake @ONLY)
install(CODE
        "
        set(DOODLE_WIX_EXE_PATH \"$<TARGET_FILE:wix_exe>\")
        set(DOODLE_PANDOC_PATH \"$<TARGET_FILE:pandoc>\")
        include(\"${CMAKE_CURRENT_BINARY_DIR}/doodle_main_install.cmake\")
        "
        COMPONENT exe_com
        )
