target_sources(
        doodle_app
        PUBLIC
        windows_proc.h
        win_network.h
        drop_manager.h

        PRIVATE
        windows_proc.cpp
        win_network.cpp
        drop_manager.cpp
)
