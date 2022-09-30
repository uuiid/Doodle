target_sources(
        doodle_app
        PUBLIC
        windows_proc.h
        win_network.h

        PRIVATE
        windows_proc.cpp
        win_network.cpp
)
