target_sources(
        doodle_lib
        PUBLIC 
        bone_weight_inference.h
        load_fbx.h
        cross_attention_bone_weight.h
        
        PRIVATE
        bone_weight_inference.cpp
        load_fbx.cpp
        cross_attention_bone_weight.cpp
)