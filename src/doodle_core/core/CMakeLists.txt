target_sources(
        doodle_core
        PUBLIC
        core_set.h
        core_sig.h
        core_help_impl.h
        doodle_lib.h
        template_util.h
        status_info.h

        PRIVATE
        core_set.cpp
        core_sig.cpp
        core_fun_util.cpp
        doodle_lib.cpp
        status_info.cpp
)
