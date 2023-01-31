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

install(CODE "
file(GLOB WIX_FILE_LISTS
        RELATIVE \${CMAKE_INSTALL_PREFIX}/wix
        \${CMAKE_INSTALL_PREFIX}/wix/*.wxs
        )
file(GLOB 7Z_FOLDER_LISTS
        LIST_DIRECTORIES true
        RELATIVE \${CMAKE_INSTALL_PREFIX}
        \${CMAKE_INSTALL_PREFIX}/*
        )

set(DOODLE_PACKAGE_FEATURE_REF_ID_LIST)
foreach (files IN LISTS WIX_FILE_LISTS)
    file(READ \${CMAKE_INSTALL_PREFIX}/wix/\${files} DOODLE_TEST_STR)
    if (DOODLE_TEST_STR MATCHES [[<Feature Id=\"([A-Za-z0-9_]+)\"]])
        list(APPEND DOODLE_PACKAGE_FEATURE_REF_ID_LIST \"<FeatureRef Id=\\\"\${CMAKE_MATCH_1}\\\"/>\")
    endif ()
endforeach ()

configure_file(${CMAKE_CURRENT_LIST_DIR}/main.wxs \${CMAKE_INSTALL_PREFIX}/wix/main.wxs)

list(REMOVE_ITEM 7Z_FOLDER_LISTS wix)
list(TRANSFORM 7Z_FOLDER_LISTS PREPEND \${CMAKE_INSTALL_PREFIX}/)
list(FILTER 7Z_FOLDER_LISTS EXCLUDE REGEX \"Doodle-\")
message(\"pack folder \${7Z_FOLDER_LISTS} to ${DOODLE_PACKAGE_NAME}\")


execute_process(
        COMMAND \"$<TARGET_FILE:wix_exe>\" extension add WixToolset.UI.wixext

        WORKING_DIRECTORY \${CMAKE_INSTALL_PREFIX}/wix
)
execute_process(
        COMMAND \"$<TARGET_FILE:wix_exe>\" build
                -o \"\${CMAKE_INSTALL_PREFIX}/${DOODLE_PACKAGE_NAME}.msi\"
                -pdb \"\${CMAKE_INSTALL_PREFIX}/wix/${DOODLE_PACKAGE_NAME}.wixpdb\"
                -pdbType full
#                -arch 64
                -cabcache \"\${CMAKE_INSTALL_PREFIX}/wix\"
                -culture zh-CN
                -cabthreads ${DOODLE_CPU_N}
                -ext WixToolset.UI.wixext
                -i \${CMAKE_INSTALL_PREFIX}
                -nologo
                \${WIX_FILE_LISTS}

        COMMAND ${SEVENZIP_BIN} a -mx2 -mmt8
        \"\${CMAKE_INSTALL_PREFIX}/${DOODLE_PACKAGE_NAME}.7z\"
        \${7Z_FOLDER_LISTS}


        WORKING_DIRECTORY \${CMAKE_INSTALL_PREFIX}/wix
        )
"
        COMPONENT exe_com
        )

