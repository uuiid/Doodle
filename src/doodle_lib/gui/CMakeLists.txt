


add_subdirectory(widgets)
target_sources(
        doodle_lib
        PUBLIC
        menu_bar.h
        layout_window.h
        setting_windows.h
        asset_library_layout.h

        PRIVATE
        menu_bar.cpp
        layout_window.cpp
        setting_windows.cpp
        asset_library_layout.cpp
)
