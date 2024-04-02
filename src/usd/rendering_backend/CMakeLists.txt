add_library(
        usd_ue_rendering
        SHARED
        usd_ue_rendering.h
)
add_subdirectory(src)
target_link_libraries(usd_ue_rendering PRIVATE usd)