target_sources(
        doodle_maya_exe
        PUBLIC
        cloth_sim.h
        export_fbx.h
        replace_file.h


        PRIVATE
        cloth_sim.cpp
        cloth_sim_main.cpp
        export_fbx.cpp
        replace_file.cpp
)