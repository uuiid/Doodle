target_sources(doodle_lib
    PUBLIC
    depth_anything_fwd.h
    doodle_depth_estimation.h

    PRIVATE
    doodle_depth_estimation.cpp
)

target_link_libraries(doodle_lib
    PUBLIC
    depths-cpp
)