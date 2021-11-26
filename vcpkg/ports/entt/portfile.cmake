if ("experimental" IN_LIST FEATURES)
    vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO skypjack/entt
        HEAD_REF experimental
    )
else()
    vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO skypjack/entt
        REF b8774b6d3858a6b72a11c1c5d88f7dc823c783b6 #v3.8.0
        SHA512 72996a8c8523dd1e77b07b984bc7fd87c46ddcda04588a75e28ef3291d354feace5a2381cb7c1e5e5e83ac7d18f7899f7cd92dff4740893a6c2b2669b12171e7
        HEAD_REF master
    )
endif()

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
    DISABLE_PARALLEL_CONFIGURE
    PREFER_NINJA
    OPTIONS
        -DENTT_BUILD_TESTING=OFF
)

vcpkg_install_cmake()

if(EXISTS ${CURRENT_PACKAGES_DIR}/cmake)
    vcpkg_fixup_cmake_targets(CONFIG_PATH cmake)
else()
    vcpkg_fixup_cmake_targets(CONFIG_PATH lib/EnTT/cmake)
endif()

file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug ${CURRENT_PACKAGES_DIR}/lib)

# Handle copyright
file(INSTALL ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
