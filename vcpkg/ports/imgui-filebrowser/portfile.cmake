
vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO AirGuanZ/imgui-filebrowser
        REF 3b3e44a686350602f5b731c9a803429b281db195 #v0.0.1
        SHA512 613b1d992fe9467c26cb1c95cc2a9f661b2c214d69921702b52facb0666e4178832a73648cb58d6278017f115b291a9819e40397c8cb302b5f598c06e85e21c3
        HEAD_REF master
)

file(COPY ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt DESTINATION ${SOURCE_PATH})
vcpkg_configure_cmake(
        SOURCE_PATH ${SOURCE_PATH}
        DISABLE_PARALLEL_CONFIGURE
        PREFER_NINJA
)

#vcpkg_cmake_install()
vcpkg_install_cmake()

file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug ${CURRENT_PACKAGES_DIR}/lib)

# Handle copyright
file(INSTALL ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
