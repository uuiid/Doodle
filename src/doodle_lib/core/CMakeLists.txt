target_sources(doodle_lib
        PUBLIC
        tree_node.h
        ContainerDevice.h
        filesystem_extend.h
        observable_container.h
        holidaycn_time.h
        scan_win_service.h
        up_auto_light_file.h
        cache_manger.h

        PRIVATE
        filesystem_extend.cpp
        holidaycn_time.cpp
        scan_win_service.cpp
        up_auto_light_file.cpp

)
add_subdirectory(scan_assets)
add_subdirectory(http)