target_sources(doodle_lib
        PUBLIC
        tree_node.h
        ContainerDevice.h
        filesystem_extend.h
        image_loader.h
        observable_container.h

        holidaycn_time.h
        bind_front_handler.h
        erase_handle_component.h

        PRIVATE
        filesystem_extend.cpp
        image_loader.cpp
        holidaycn_time.cpp


        )