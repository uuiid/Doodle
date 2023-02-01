file(GLOB WIX_FILE_LISTS
        RELATIVE ${CMAKE_INSTALL_PREFIX}/wix
        ${CMAKE_INSTALL_PREFIX}/wix/*.wxs
        )
file(GLOB 7Z_FOLDER_LISTS
        LIST_DIRECTORIES true
        RELATIVE ${CMAKE_INSTALL_PREFIX}
        ${CMAKE_INSTALL_PREFIX}/*
        )

set(DOODLE_PACKAGE_FEATURE_REF_ID_LIST)
foreach (files IN LISTS WIX_FILE_LISTS)
    file(READ ${CMAKE_INSTALL_PREFIX}/wix/${files} DOODLE_TEST_STR)
    if (DOODLE_TEST_STR MATCHES [[<Feature Id="([A-Za-z0-9_]+)"]])
        string(REGEX MATCHALL [[<Feature Id="([A-Za-z0-9_]+)"]] DOODLE_TMP_LIST ${DOODLE_TEST_STR})
        foreach (TMP_STR IN LISTS DOODLE_TMP_LIST)
            string(REGEX MATCH [[<Feature Id="([A-Za-z0-9_]+)"]] _ ${TMP_STR})
            list(APPEND DOODLE_PACKAGE_FEATURE_REF_ID_LIST "<FeatureRef Id=\"${CMAKE_MATCH_1}\"/>")
        endforeach ()
    endif ()
endforeach ()


list(JOIN DOODLE_PACKAGE_FEATURE_REF_ID_LIST " " DOODLE_PACKAGE_FEATURE_REF_ID_LIST)

configure_file(${CMAKE_CURRENT_LIST_DIR}/main.wxs ${CMAKE_INSTALL_PREFIX}/wix/main.wxs)
list(APPEND WIX_FILE_LISTS main.wxs)
list(REMOVE_DUPLICATES WIX_FILE_LISTS)

execute_process(
        COMMAND ${DOODLE_PANDOC_PATH} -s ${PROJECT_SOURCE_DIR}/LICENSE -f textile -o ./wix/LICENSE.rtf
        WORKING_DIRECTORY \${CMAKE_INSTALL_PREFIX}
)

#execute_process(COMMAND ${CMAKE_COMMAND}
#        -D CMAKE_CURRENT_LIST_DIR=@CMAKE_CURRENT_LIST_DIR@
#        -D CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
#        -P @CMAKE_CURRENT_LIST_DIR@/doodle_wix_config.cmake
#        )

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
