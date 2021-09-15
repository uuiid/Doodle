vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
        FEATURES
        core IMGUIWRAP_BUILD_CORE
        glfw-backed IMGUIWRAP_BUILD_GLFW_BACKED
        )

if (IMGUIWRAP_BUILD_CORE)
    vcpkg_from_github(
            OUT_SOURCE_PATH SOURCE_PATH
            REPO kfsone/imguiwrap
            REF v1.2.2
            SHA512 0d7152cc299faf7d06a8f46a25d03a128d1a162ed24b92b81396d8f51ddea44e038c1e60bfa3fbfc24338191c78f989c96ce85dc677a7ee656dfefe78dd730d3
            HEAD_REF main
            PATCHES 0001-only-header.patch
    )
else ()
    vcpkg_from_github(
            OUT_SOURCE_PATH SOURCE_PATH
            REPO kfsone/imguiwrap
            REF v1.2.2
            SHA512
            HEAD_REF main
    )
endif ()


file(COPY ${CMAKE_CURRENT_LIST_DIR}/imguiwrap-config.cmake.in DESTINATION ${SOURCE_PATH})
file(COPY ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt DESTINATION ${SOURCE_PATH})


vcpkg_configure_cmake(
        SOURCE_PATH ${SOURCE_PATH}
        PREFER_NINJA
        OPTIONS
        -DIMGUIWRAP_BUILD_CORE=${IMGUIWRAP_BUILD_CORE}
)

vcpkg_install_cmake()

vcpkg_copy_pdbs()
vcpkg_fixup_cmake_targets()

file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug)
file(INSTALL ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
