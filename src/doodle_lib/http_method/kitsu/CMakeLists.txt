target_sources(
        doodle_lib
        
        PUBLIC
        kitsu_front_end.h
        epiboly.h
        kitsu_reg_url.h
        kitsu_result.h
        preview.h
        comment.h
        computing_time.h


        PRIVATE
        kitsu_reg_url.cpp
        task.cpp
        user.cpp
        kitsu_front_end.cpp
        project.cpp
        epiboly.cpp
        assets.cpp
        preview.cpp
        login.cpp
        config.cpp
        time.cpp
        base_data.cpp
        sequences.cpp
        comment.cpp
        pictures.cpp
        playlists.cpp
        attachment_files.cpp
        metadata_descriptors.cpp
        entities.cpp
        schedule_items.cpp
        auth.cpp
        actions.cpp
        notifications.cpp
        shots.cpp
        movies.cpp
        file_status.cpp
        output_types.cpp
        working_file.cpp
        casting.cpp
        auto_task.cpp
        compose_video.cpp
        auth_outsource.cpp
        computing_time.cpp

)

# 这个文件夹中所有的源文件都要使用限制池构建
set_source_files_properties(
        DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        PROPERTIES JOB_POOL sql_query_pool
)