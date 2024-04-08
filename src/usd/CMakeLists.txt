find_package(usd CONFIGS
        HINTS ${PXR_USD_ROOT}
)
if (TARGET usd)
    add_subdirectory(rendering_backend)
endif ()