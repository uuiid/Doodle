target_sources(
        doodle_lib
        PUBLIC
        model_library.h

        PRIVATE
        assets.cpp
        assets_tree.cpp
        thumbnail.cpp
        label.cpp
        context.cpp
        ai_image.cpp
)

set_source_files_properties(
        assets.cpp
        assets_tree.cpp
        thumbnail.cpp
        label.cpp
        context.cpp
        ai_image.cpp

        TARGET_DIRECTORY doodle_lib
        PROPERTIES JOB_POOL_COMPILE sql_query_pool
)