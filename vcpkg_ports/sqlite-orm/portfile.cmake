# header-only library

vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO fnc12/sqlite_orm
        REF 92b7959b1d54dd4d8c6038f6fcd18fa899bd30b4
        SHA512 c9eecec3da55cbb9d5f6c5aec53126456339a5a410fc0f437c052599704f9e7051c5f176386e133897191d8e4e6530d9d85c83f047483b1e4cd0cd39e2f962c7
        HEAD_REF dev
        PATCHES
        fix-dependency.patch
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
        -DSQLITE_ORM_ENABLE_CXX_17=ON
        -DSQLITE_ORM_ENABLE_CXX_20=ON
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(PACKAGE_NAME SqliteOrm CONFIG_PATH lib/cmake/SqliteOrm)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/lib")

file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)