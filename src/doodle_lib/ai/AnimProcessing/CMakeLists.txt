target_sources(
        doodle_lib
        PUBLIC 
        InverseKinematics.h
        TrajectoryCorrector.h
        Utility.h
        
        PRIVATE
        InverseKinematics.cpp
        TrajectoryCorrector.cpp
        Utility.cpp
        )