# header-only library

vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO yanyiwu/limonp
        REF 8c50bfd6fc05c51e89ba36b02aad6b9531995d73
        SHA512 2dbf8543742d705d1067e02939828edbefd6d1f62367b5fed45e7fbc67a62179ae84bff2ce5149c5f06cd0180161d095632a93100772ba5410b549d382ff3225
        HEAD_REF master

)


vcpkg_cmake_configure(
        SOURCE_PATH "${SOURCE_PATH}"
        OPTIONS
        -DENABLE_UNIT_TESTS=OFF
)

vcpkg_cmake_install()

# vcpkg_cmake_config_fixup(PACKAGE_NAME cppjieba CONFIG_PATH lib/cmake/cppjieba)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug")
# file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/lib")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")