target_sources(
        doodle_core
        PUBLIC
        chrono_fmt.h
        boost_uuid_warp.h
        enum_template_tool.h
        json_warp.h
        maya_exe_out.h

        PRIVATE
        maya_exe_out.cpp
)