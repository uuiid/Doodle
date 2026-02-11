target_sources(doodle_lib
        PUBLIC
        tree_node.h
        ContainerDevice.h
        filesystem_extend.h
        observable_container.h
        holidaycn_time.h
        cache_manger.h
        socket_io.h
        base64.h
        fbx_file.h
        alembic_file.h
        asyn_task.h
        entity_path.h
        ffmpeg_video.h
        generate_text_video.hpp
        crashpad.h
        
        PRIVATE
        filesystem_extend.cpp
        holidaycn_time.cpp
        socket_io.cpp
        base64.cpp
        fbx_file.cpp
        alembic_file.cpp
        entity_path.cpp
        ffmpeg_video.cpp
        generate_text_video.cpp
        crashpad.cpp
)
add_subdirectory(http)
add_subdirectory(socket_io)

set_source_files_properties(
        entity_path.cpp

        TARGET_DIRECTORY doodle_lib
        PROPERTIES JOB_POOL_COMPILE sql_query_pool
)