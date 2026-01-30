vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO EpicGames/MetaHuman-DNA-Calibration
        REF 8297ff1bd6f1692a6e221aa8395a66486e5a53c9
        SHA512 0
        HEAD_REF main
        PATCHES
        0001-python.patch
)
vcpkg_cmake_configure(
        SOURCE_PATH ${SOURCE_PATH}
        OPTIONS
        -DBUILD_TEST=OFF
)

vcpkg_cmake_install()
vcpkg_copy_pdbs()

vcpkg_cmake_config_fixup()

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")