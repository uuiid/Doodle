vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO mlc-ai/tokenizers-cpp
    REF "v${VERSION}"
    SHA512 c319b2c063ed81e380a4a375bf3432ae7cd70558b59818655c54fd9b0cf6f03cdddd72845a76b455bf4cc0ea05aef0b0917ea2cc8135fd4539e890fc5afc7431
    HEAD_REF main
)

# tokenizers-cpp needs Rust (cargo) to build its C binding library.
# See https://www.rust-lang.org/tools/install
vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DMLC_ENABLE_SENTENCEPIECE_TOKENIZER=OFF
        -DSPM_ENABLE_SHARED=OFF
        -DSPM_ENABLE_TCMALLOC=OFF
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(PACKAGE_NAME tokenizers_cpp CONFIG_PATH lib/cmake/tokenizers_cpp)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
