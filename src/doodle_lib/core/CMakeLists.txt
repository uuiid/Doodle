target_sources(doodle_lib
        PUBLIC
        tree_node.h
        ContainerDevice.h
        filesystem_extend.h
        observable_container.h
        holidaycn_time.h
        erase_handle_component.h
        scan_win_service.h
        thread_copy_io.h
        up_auto_light_file.h

        PRIVATE
        filesystem_extend.cpp
        holidaycn_time.cpp
        scan_win_service.cpp
        thread_copy_io.cpp
        up_auto_light_file.cpp

)
add_subdirectory(scan_assets)
add_subdirectory(http)