target_sources(
        doodle_lib
        PUBLIC
        maya_exe.h
        ue_exe.h
        import_and_render_ue.h
        async_read_pipe.h

        PRIVATE
        maya_exe.cpp
        ue_exe.cpp
        import_and_render_ue.cpp
        async_read_pipe.cpp
)