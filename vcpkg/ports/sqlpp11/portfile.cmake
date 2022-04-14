vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO rbock/sqlpp11
        REF b50cc454b62e88bf03d681f731cdd98c23a42ffb # 0.60
        SHA512 168baa5d5dd6a4119623cc832243c5d7d33ab9d165f5922997d612f34d6e829764f3d4d0ab7dac780f560e7ce6b92aa1632f42afd65227cae6e584c06a52f9f0
        HEAD_REF master
        PATCHES
        0001-fix_ddl.patch
        0002-fix-sqlite3.patch
        0003-fix-sqlite3-config.patch
)

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
        FEATURES
        "mysql-connector" BUILD_MYSQL_CONNECTOR
        "sqlite3-connector" BUILD_SQLITE3_CONNECTOR
        )

# Use sqlpp11's own build process, skipping tests
vcpkg_configure_cmake(
        SOURCE_PATH ${SOURCE_PATH}
        PREFER_NINJA
        OPTIONS
        -DBUILD_TESTING:BOOL=OFF
        -DUSE_SYSTEM_DATE:BOOL=ON
        ${FEATURE_OPTIONS}
)

vcpkg_install_cmake()

# Move CMake config files to the right place
vcpkg_fixup_cmake_targets(CONFIG_PATH lib/cmake/Sqlpp11 TARGET_PATH share/${PORT})

# Delete redundant and unnecessary directories
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug ${CURRENT_PACKAGES_DIR}/lib ${CURRENT_PACKAGES_DIR}/cmake ${CURRENT_PACKAGES_DIR}/include/date)

# Move python script from bin directory
file(COPY ${CURRENT_PACKAGES_DIR}/bin/sqlpp11-ddl2cpp DESTINATION ${CURRENT_PACKAGES_DIR}/scripts)
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/bin/)

# Handle copyright
file(INSTALL ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
