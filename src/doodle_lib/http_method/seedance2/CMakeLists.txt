target_sources(
        doodle_lib
        PUBLIC

        reg.h

        PRIVATE
        assets_entity_item.cpp
        assets_entity.cpp
        group.cpp
        task.cpp
)

 

set_source_files_properties(
        assets_entity_item.cpp
        assets_entity.cpp
        group.cpp
        task.cpp

        TARGET_DIRECTORY doodle_lib
        PROPERTIES JOB_POOL_COMPILE sql_query_pool
)