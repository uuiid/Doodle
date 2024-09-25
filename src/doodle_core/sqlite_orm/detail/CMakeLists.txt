target_sources(
        doodle_core
        PUBLIC
        init_project.h
        assets_type_enum.h
        std_chrono_duration.h
        std_chrono_time_point.h
        std_chrono_zoned_time.h
        std_filesystem_path_orm.h
        uuid_to_blob.h

        PRIVATE
        init_project.cpp
)