target_sources(
        doodle_core
        PUBLIC
        std_chrono_duration.h
        std_chrono_time_point.h
        std_chrono_zoned_time.h
        std_filesystem_path_orm.h
        nlohmann_json.h
        uuid_to_blob.h
        macro.h
        sqlite_database_impl.h
        std_vector_string.h

        PRIVATE
        sqlite_database_impl.cpp
)