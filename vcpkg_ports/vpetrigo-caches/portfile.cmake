vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO vpetrigo/caches
        REF "v${VERSION}"
        SHA512 fb127cb2cf5ae9f635617816b3d02dd90740f0453daf121dc8a54ddea7aabe7bb13750be3812697ec904b7a11d447b602568f7b328d245e360126573d89597b8
        HEAD_REF master
        PATCHES
        0001-fix-cmake-install-target.patch
)


vcpkg_cmake_configure(
        SOURCE_PATH ${SOURCE_PATH}
        OPTIONS
        -DBUILD_TEST=OFF
        -DINSTALL_CACHES=ON
)
vcpkg_cmake_install()

vcpkg_cmake_config_fixup(
        PACKAGE_NAME caches
        CONFIG_PATH lib/caches
)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug" "${CURRENT_PACKAGES_DIR}/lib")
# Handle copyright
file(INSTALL "${SOURCE_PATH}/LICENSE.md" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
