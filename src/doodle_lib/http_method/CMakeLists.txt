target_sources(
        doodle_lib
        PUBLIC
        computer.h
        computing_time.h
        dingding_attendance.h
        file_association.h
        tool_version.h
        up_file.h
        http_jwt_fun.h
        seed_email.h

        PRIVATE
        computer.cpp
        computing_time.cpp
        dingding_attendance.cpp
        file_association.cpp
        computer_reg_data.cpp
        tool_version.cpp
        up_file.cpp
        http_jwt_fun.cpp
        seed_email.cpp
)

add_subdirectory(kitsu)
add_subdirectory(model_library)
add_subdirectory(local)
add_subdirectory(other)