file(GLOB WIX_FILE_LISTS
        RELATIVE ${CMAKE_INSTALL_PREFIX}/wix
        ${CMAKE_INSTALL_PREFIX}/wix/*.wxs
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