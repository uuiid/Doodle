target_sources(
        doodle_lib
        PUBLIC
        computer.h
        task_info.h
        file_exists.h
        computing_time.h
        dingding_attendance.h
        file_association.h
        share_fun.h
        kitsu_front_end_reg.h

        PRIVATE
        computer.cpp
        task_info.cpp
        file_exists.cpp
        computing_time.cpp
        dingding_attendance.cpp
        file_association.cpp
        share_fun.cpp
        computer_reg_data.cpp
        kitsu_front_end_reg.cpp

)

add_subdirectory(kitsu)
add_subdirectory(model_library)