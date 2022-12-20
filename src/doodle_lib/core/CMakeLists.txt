target_sources(doodle_lib
        PUBLIC
        authorization.h
        tree_node.h
        ContainerDevice.h
        filesystem_extend.h
        image_loader.h
        observable_container.h

        holidaycn_time.h
        xlsx.h

        PRIVATE
        filesystem_extend.cpp
        image_loader.cpp
        holidaycn_time.cpp
        authorization.cpp
        xlsx.cpp


        )