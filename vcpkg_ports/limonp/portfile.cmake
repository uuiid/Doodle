# header-only library

vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO yanyiwu/limonp
        REF "v${VERSION}"
        SHA512 3cf1103ea04610dce471c294b052e1bbef84a401c2a10d4251357423e302f4d3a2f3518c0ef332be009f1093f8a1b0bbfa7f857f7f09181ddb899246d59ff139
        HEAD_REF master

)

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
        FEATURES
        test BUILD_TESTING
        example BUILD_EXAMPLES
)

vcpkg_cmake_configure(
        SOURCE_PATH "${SOURCE_PATH}"
        OPTIONS
        ${FEATURE_OPTIONS}
)

vcpkg_cmake_install()

# vcpkg_cmake_config_fixup(PACKAGE_NAME cppjieba CONFIG_PATH lib/cmake/cppjieba)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug")
# file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/lib")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")