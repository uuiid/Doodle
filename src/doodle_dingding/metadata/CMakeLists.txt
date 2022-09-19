target_sources(doodle_dingding
        PUBLIC
        access_token.h
        department.h
        request_base.h
        user_dd.h
        attendance.h
        dingding_tool.h

        PRIVATE
        access_token.cpp
        department.cpp
        request_base.cpp
        user_dd.cpp
        attendance.cpp
        dingding_tool.cpp

        )
