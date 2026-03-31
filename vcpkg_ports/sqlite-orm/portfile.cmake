# header-only library

vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO fnc12/sqlite_orm
        REF e18ea01811c9abc6fd36841609e9bcdb74f7dce9
        SHA512 385c9302b28c3cdd75cb5b7a8eaf51d795fa3c65c477be053c6cc5c4071de04f8cd2a418f7ed36aeaf40f566e8c2227bfb1f45d1042f4e0c28b7470b5aded516
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