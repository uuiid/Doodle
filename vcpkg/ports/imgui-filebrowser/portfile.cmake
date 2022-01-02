
vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO AirGuanZ/imgui-filebrowser
        REF 3b3e44a686350602f5b731c9a803429b281db195 #v0.0.1
        SHA512 0
        HEAD_REF master
)

vcpkg_configure_cmake(
        SOURCE_PATH ${SOURCE_PATH}
        DISABLE_PARALLEL_CONFIGURE
        PREFER_NINJA
)

file(COPY ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt DESTINATION ${SOURCE_PATH})
vcpkg_cmake_install()


file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug ${CURRENT_PACKAGES_DIR}/lib)

# Handle copyright
file(INSTALL ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
