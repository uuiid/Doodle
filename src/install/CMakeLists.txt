find_package(Pandoc)

set(DOODLE_PACKAGE_VENDOR uuiid)
configure_file(main.iss main.iss)

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/main.iss
        DESTINATION wix
        COMPONENT exe_com)


include(ProcessorCount)
ProcessorCount(DOODLE_CPU_N)
find_package(SevenZip)

configure_file(doodle_main_install.cmake doodle_main_install.cmake @ONLY)
install(CODE
        "
        set(DOODLE_INNO_EXE_PATH \"$<TARGET_FILE:Inno_exe>\")
        set(DOODLE_PANDOC_PATH \"$<TARGET_FILE:pandoc>\")
        include(\"${CMAKE_CURRENT_BINARY_DIR}/doodle_main_install.cmake\")
        "
        COMPONENT exe_com
)
message("${CMAKE_INSTALL_PREFIX} ${PROJECT_SOURCE_DIR}")