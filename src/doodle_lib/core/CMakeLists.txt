target_sources(
        doodle_lib
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
        core_set.h
        doodle_lib.h
        template_util.h
        file_sys.h
        app_base.h
        global_function.h
        app_service.h
        http_client_core.h
        co_queue.h
        authorization.h
        cancellation_signals.h

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
        core_set.cpp
        doodle_lib.cpp
        file_sys.cpp
        app_base.cpp
        app_service.cpp
        http_client_core.cpp
        co_queue.cpp
        authorization.cpp
        global_function.cpp
        cancellation_signals.cpp

)
add_subdirectory(http)
add_subdirectory(socket_io)
add_subdirectory(bcrypt)

set_source_files_properties(
        entity_path.cpp

        TARGET_DIRECTORY doodle_lib
        PROPERTIES JOB_POOL_COMPILE sql_query_pool
)