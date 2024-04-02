add_library(
        usd_ue_rendering
        SHARED
        usd_ue_rendering.h
)
add_subdirectory(src)
target_link_libraries(usd_ue_rendering PRIVATE ${PXR_LIBRARIES})

set(PLUG_INFO_LIBRARY_PATH "../usd_ue_rendering.dll")
set(PLUG_INFO_RESOURCE_PATH "resources")
set(PLUG_INFO_ROOT "..")
configure_file(pluginfo.json resources/PlugInfo.json @ONLY)
