vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO electronicarts/dem-bones
        REF bb4ea9f391c4be9abe4f85a407c2c47979bad959 # 1.2.1
        SHA512 db8e050928ecf186ea2c5946ac5f37a2eae08011863524d8997b04e65cba439492b76a51de3fd4827263c6bf36d8353701657d7a4912b1786dd7deb9b2194c45
        HEAD_REF master
)

file(COPY "${CMAKE_CURRENT_LIST_DIR}/Config.cmake.in" DESTINATION "${SOURCE_PATH}")
file(COPY "${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt" DESTINATION "${SOURCE_PATH}")

vcpkg_cmake_configure(
        SOURCE_PATH ${SOURCE_PATH}
)
vcpkg_cmake_install()

if (EXISTS "${CURRENT_PACKAGES_DIR}/cmake")
    vcpkg_cmake_config_fixup(CONFIG_PATH cmake)
else ()
    vcpkg_cmake_config_fixup(CONFIG_PATH lib/dem-bones/cmake)
endif ()
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug" "${CURRENT_PACKAGES_DIR}/lib")

# Handle copyright
file(INSTALL "${SOURCE_PATH}/LICENSE.md" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
