target_sources(
        doodle_core

        PUBLIC
        time_point_info.h
        boost_lcl_time_point_adaptation.h
        user_set_data.h

        PRIVATE
        time_point_info.cpp
        user_set_data.cpp
)
