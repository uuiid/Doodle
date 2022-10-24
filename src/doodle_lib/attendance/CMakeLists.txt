target_sources(
        doodle_lib
        PUBLIC
        attendance_interface.h
        attendance_rule.h
        attendance_dingding.h

        PRIVATE
        attendance_interface.cpp
        attendance_rule.cpp
        attendance_dingding.cpp
)