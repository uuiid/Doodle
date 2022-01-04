
vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO uuiid/imgui-filebrowser
        REF b5a056366ff95ef5a1d277a62387f7d2a93db6b1 #v0.0.1
        SHA512 835a3020666ea1abbb5133416c10b3ff9062a9f996bdb90abc43fb8cfe95a639a7b88e348873dd3608537af1ba836065e24f9f81e6b24072920de4671b110b69
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
