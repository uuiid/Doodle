target_sources(doodle_lib
        PUBLIC
        tree_node.h
        ContainerDevice.h
        filesystem_extend.h
        image_loader.h
        observable_container.h

        holidaycn_time.h
        erase_handle_component.h
        scan_win_service.h
        down_auto_light_anim_file.h
        thread_copy_io.h
        up_auto_light_file.h
        auto_light_render_video.h

        PRIVATE
        filesystem_extend.cpp
        image_loader.cpp
        holidaycn_time.cpp
        scan_win_service.cpp
        down_auto_light_anim_file.cpp
        thread_copy_io.cpp
        up_auto_light_file.cpp
        auto_light_render_video.cpp

)
add_subdirectory(scan_assets)
add_subdirectory(http)