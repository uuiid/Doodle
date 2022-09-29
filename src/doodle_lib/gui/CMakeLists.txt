


add_subdirectory(widgets)
target_sources(
        doodle_lib
        PUBLIC
        menu_bar.h

        PRIVATE
        menu_bar.cpp
)
