execute_process(COMMAND ${CMAKE_COMMAND}
        -D CMAKE_CURRENT_LIST_DIR=@CMAKE_CURRENT_LIST_DIR@
        -D CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
        -P @CMAKE_CURRENT_LIST_DIR@/doodle_wix_config.cmake
        )

list(REMOVE_ITEM 7Z_FOLDER_LISTS wix)
list(TRANSFORM 7Z_FOLDER_LISTS PREPEND ${CMAKE_INSTALL_PREFIX}/)
list(FILTER 7Z_FOLDER_LISTS EXCLUDE REGEX "Doodle-")
message("pack folder ${7Z_FOLDER_LISTS} to @DOODLE_PACKAGE_NAME@")

execute_process(
        COMMAND @SEVENZIP_BIN@ a -mx2 -mmt8
        "${CMAKE_INSTALL_PREFIX}/@DOODLE_PACKAGE_NAME@.7z"
        ${7Z_FOLDER_LISTS}


        WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/wix
)


execute_process(
        COMMAND "${DOODLE_WIX_EXE_PATH}" extension add WixToolset.UI.wixext

        WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/wix
)
execute_process(
        COMMAND "${DOODLE_WIX_EXE_PATH}" build
        -o "${CMAKE_INSTALL_PREFIX}/@DOODLE_PACKAGE_NAME@.msi"
        -pdb "${CMAKE_INSTALL_PREFIX}/wix/@DOODLE_PACKAGE_NAME@.wixpdb"
        -pdbType full
        #                -arch 64
        -cabcache "${CMAKE_INSTALL_PREFIX}/wix"
        -culture zh-CN
        -cabthreads @DOODLE_CPU_N@
        -ext WixToolset.UI.wixext
        -i ${CMAKE_INSTALL_PREFIX}/wix
        -nologo
        ${WIX_FILE_LISTS}

        WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/wix
)
