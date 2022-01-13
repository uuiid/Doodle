vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO rbock/sqlpp11
        REF 2bc89b34ad3cc37b6bca9a44a3529ff2d8fe211f # 0.60
        SHA512 6e2496959749422987aca21f333abb01648702b85e02acc711bbac398ca6a67d8be93a3d89fc1f8bad5446865725ff9bcc053e6229cb34627120b59469426266
        HEAD_REF master
        PATCHES
        0001-fix_ddl.patch
        0001-fix-sqlite3.patch
        0001-fix-sqlite3-config.patch
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
