target_sources(
        doodle_lib
        PUBLIC
        attendance_interface.h
        attendance_rule.h

        PRIVATE
        attendance_interface.cpp
        attendance_rule.cpp
)