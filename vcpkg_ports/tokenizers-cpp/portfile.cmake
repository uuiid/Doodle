vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO mlc-ai/tokenizers-cpp
    REF c586c52f93f7b060753bd2388eb96a105cb7374d
    SHA512 8976bd30881b100dc0eb83a3cd02d2c1c97f593b64a74add17dc67ef2e9678a03fc00387f322a97f746084061e6b6b1f652247110249f021e9a83c9468e90e68
    HEAD_REF main
    PATCHES
        0001-fix-sub-mod-add-install.patch
)

# tokenizers-cpp needs Rust (cargo) to build its C binding library.
# See https://www.rust-lang.org/tools/install
vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DMLC_ENABLE_SENTENCEPIECE_TOKENIZER=OFF
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(PACKAGE_NAME tokenizers_cpp CONFIG_PATH lib/cmake/tokenizers_cpp)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
