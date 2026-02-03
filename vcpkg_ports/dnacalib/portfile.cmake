vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO EpicGames/MetaHuman-DNA-Calibration
        REF 8297ff1bd6f1692a6e221aa8395a66486e5a53c9
        SHA512 f32ad71cf81999497ea0fece394e82ecb59862952c925d2770526f824b0b7a8d6b8f6629df348d7e05af236b90bc3444fe140abe4d8426519789294cb4713b5a
        HEAD_REF main
        PATCHES
        0001-python.patch
        0002-fix-install-dir.patch
)
vcpkg_cmake_configure(
        SOURCE_PATH ${SOURCE_PATH}/dnacalib
        OPTIONS
)

vcpkg_cmake_install()
vcpkg_copy_pdbs()

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")