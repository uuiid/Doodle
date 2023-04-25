target_sources(doodle_maya_lib
        PUBLIC
        comm_check_scenes.h
        create_sim_cloth.h
        dem_cloth_to_fbx.h
        reference_attr_setting.h

        PRIVATE
        comm_check_scenes.cpp
        create_sim_cloth.cpp
        dem_cloth_to_fbx.cpp
        reference_attr_setting.cpp

        )
