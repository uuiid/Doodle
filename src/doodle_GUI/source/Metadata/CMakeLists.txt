add_subdirectory(View)
add_subdirectory(Model)
target_sources(
        doodle
        PRIVATE
        MetadataWidget.cpp
)