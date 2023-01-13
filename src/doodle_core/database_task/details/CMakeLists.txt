target_sources(doodle_core
        PUBLIC
        com_data.h
        update_ctx.h
        macro.h
        user.h
        work_task.h
        tool.h
        database.h

        PRIVATE
        com_data.cpp
        update_ctx.cpp
        user.cpp
        work_task.cpp
        database.cpp
        )
