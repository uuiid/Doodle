target_sources(
        doodle_lib
        PUBLIC
        computer.h
        task_info.h
        computing_time.h
        dingding_attendance.h
        file_association.h
        kitsu_front_end_reg.h
        tool_version.h
        local_setting.h

        PRIVATE
        computer.cpp
        task_info.cpp
        computing_time.cpp
        dingding_attendance.cpp
        file_association.cpp
        computer_reg_data.cpp
        kitsu_front_end_reg.cpp
        tool_version.cpp
        local_setting.cpp

)

add_subdirectory(kitsu)
add_subdirectory(model_library)