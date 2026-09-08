vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO Geekgineer/Depths-CPP
    REF b5464894049a77b2caf210dfc1b4c15652194377
    SHA512 91482fb1cf35833303efa17c242a24ba8d576cdc1d06bf092bfc40615b1f57904b5b349155cba463069d50266ff8d2c0bb01d13d368e1531cdfeb12f965d5f71
    HEAD_REF main
)

# 复制自定义 CMakeLists.txt（header-only 库的安装规则）
file(COPY "${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt" DESTINATION "${SOURCE_PATH}")

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(PACKAGE_NAME depths-cpp CONFIG_PATH lib/cmake/depths-cpp)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug")

file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)