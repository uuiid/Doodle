target_sources(
        doodle_lib
        PUBLIC
        computer.h
        computing_time.h
        dingding_attendance.h
        file_association.h
        kitsu_front_end_reg.h
        tool_version.h
        up_file.h

        PRIVATE
        computer.cpp
        computing_time.cpp
        dingding_attendance.cpp
        file_association.cpp
        computer_reg_data.cpp
        kitsu_front_end_reg.cpp
        tool_version.cpp
        up_file.cpp
)

add_subdirectory(kitsu)
add_subdirectory(model_library)
add_subdirectory(local)