target_sources(
        doodle_lib
        PUBLIC 
        Constants.h
        Matrix.h
        Quaternion.h
        Scalar.h
        SIMD.h
        Transform.h
        Types.h
        Vector.h
        
        
        
        PRIVATE
        Matrix.cpp
        Quaternion.cpp
        Transform.cpp
        Types.cpp
        Vector.cpp
        )


