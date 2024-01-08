target_sources(
        doodle_lib
        PUBLIC
        maya_exe.h
        ue_exe.h
        import_and_render_ue.h

        PRIVATE
        maya_exe.cpp
        ue_exe.cpp
        import_and_render_ue.cpp
)