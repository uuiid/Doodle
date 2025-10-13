target_sources(
        doodle_lib
        PUBLIC 
        bone_weight_inference.h
        load_fbx.h
        
        PRIVATE
        bone_weight_inference.cpp
        load_fbx.cpp
)