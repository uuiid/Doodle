add_subdirectory(sqlite)

target_sources(
        doodle_lib
        PUBLIC
        computer.h
        task_info.h
        task_server.h
        http_snapshot.h
        file_exists.h
        task_sqlite_server.h
        computing_time.h
        dingding_attendance.h
        user_http.h
        file_association.h
        share_fun.h

        PRIVATE
        computer.cpp
        task_info.cpp
        task_server.cpp
        http_snapshot.cpp
        file_exists.cpp
        task_sqlite_server.cpp
        computing_time.cpp
        dingding_attendance.cpp
        user_http.cpp
        file_association.cpp
        share_fun.cpp

)