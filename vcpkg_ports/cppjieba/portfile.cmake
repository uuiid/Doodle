# header-only library

vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO yanyiwu/cppjieba
        REF "v${VERSION}"
        SHA512 a01fbed1729b1b53d2ffc7f5882736ae46e877610dfbdcfa49308244d4127959a397d1da861f38ff95a573d875d4f51e87ec19eba9810d176edf1a47b446adcf
        HEAD_REF master

        PATCHES
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
        -DCPPJIEBA_TOP_LEVEL_PROJECT=OFF
)

vcpkg_cmake_install()

# vcpkg_cmake_config_fixup(PACKAGE_NAME cppjieba CONFIG_PATH lib/cmake/cppjieba)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug")
# file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/lib")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
# 复制 ${SOURCE_PATH}/dict 到 ${CURRENT_PACKAGES_DIR}/tools/cppjieba/dict
file(COPY "${SOURCE_PATH}/dict" DESTINATION "${CURRENT_PACKAGES_DIR}/tools/cppjieba")