


add_subdirectory(widgets)
target_sources(
        doodle_lib
        PUBLIC
        menu_bar.h
        layout_window.h
        setting_windows.h

        PRIVATE
        menu_bar.cpp
        layout_window.cpp
        setting_windows.cpp
)
