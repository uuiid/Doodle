add_subdirectory(sqlite)

target_sources(
        doodle_lib
        PUBLIC
        computer.h
        task_info.h
        task_server.h
        file_exists.h
        computing_time.h
        dingding_attendance.h
        user_http.h
        file_association.h
        share_fun.h
        model_base.h

        PRIVATE
        computer.cpp
        task_info.cpp
        task_server.cpp
        file_exists.cpp
        computing_time.cpp
        dingding_attendance.cpp
        user_http.cpp
        file_association.cpp
        share_fun.cpp
        computer_reg_data.cpp
        model_base.cpp

)

add_subdirectory(kitsu)