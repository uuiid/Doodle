target_sources(
        doodle_cloud_drive_lib
        PUBLIC
        cloud_fetch_data.h
        cloud_fetch_placeholders.h
        cloud_convert_to_placeholder.h

        PRIVATE
        cloud_fetch_data.cpp
        cloud_fetch_placeholders.cpp
        cloud_convert_to_placeholder.cpp
)