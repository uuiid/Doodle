target_sources(
        doodle_lib
        PUBLIC
        computer.h
        dingding_attendance.h
        tool_version.h
        up_file.h
        http_jwt_fun.h
        seed_email.h
        kitsu.h

        PRIVATE
        computer.cpp
        dingding_attendance.cpp
        computer_reg_data.cpp
        tool_version.cpp
        up_file.cpp
        http_jwt_fun.cpp
        seed_email.cpp
        kitsu.cpp
)

add_subdirectory(kitsu)
add_subdirectory(model_library)
add_subdirectory(local)
add_subdirectory(other)
# add_subdirectory1(ai)