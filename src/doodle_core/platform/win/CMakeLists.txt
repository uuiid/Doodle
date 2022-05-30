target_sources(doodle_core
        PUBLIC
        drop_manager.h
        list_drive.h
        register_file_type.h
        windows_alias.h
        wnd_proc.h

        PRIVATE
        drop_manager.cpp
        list_drive.cpp
        register_file_type.cpp
        wnd_proc.cpp

        )
