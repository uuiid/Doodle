target_sources(
        doodle_cloud_drive_lib

        PUBLIC
        cloud_provider_registrar.h
        directory_watcher.h

        PRIVATE
        cloud_provider_registrar.cpp
        directory_watcher.cpp
)