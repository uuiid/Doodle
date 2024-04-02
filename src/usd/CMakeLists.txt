find_package(usd CONFIGS)
if (TARGET usd)
    add_subdirectory(rendering_backend)
endif ()