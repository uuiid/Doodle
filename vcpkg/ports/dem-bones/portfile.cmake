vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO electronicarts/dem-bones
        REF bb4ea9f391c4be9abe4f85a407c2c47979bad959 # 1.2.1
        SHA512
        HEAD_REF master
)


# Use sqlpp11's own build process, skipping tests
vcpkg_configure_cmake(
        SOURCE_PATH ${SOURCE_PATH}
        PREFER_NINJA
        OPTIONS
        -DBUILD_TESTING:BOOL=OFF
        -DUSE_SYSTEM_DATE:BOOL=ON
)
file(COPY "${CMAKE_CURRENT_LIST_DIR}/Config.cmake.in" DESTINATION "${SOURCE_PATH}")
file(COPY "${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt" DESTINATION "${SOURCE_PATH}")

vcpkg_cmake_configure(
        SOURCE_PATH ${SOURCE_PATH}
)
vcpkg_cmake_install()

if(EXISTS "${CURRENT_PACKAGES_DIR}/cmake")
    vcpkg_cmake_config_fixup(CONFIG_PATH cmake)
else()
    vcpkg_cmake_config_fixup(CONFIG_PATH lib/dem_bones/cmake)
endif()
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug" "${CURRENT_PACKAGES_DIR}/lib")

# Handle copyright
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
