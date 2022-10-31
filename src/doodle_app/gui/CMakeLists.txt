target_sources(doodle_app
        PUBLIC
        get_input_dialog.h
        main_menu_bar.h
        main_proc_handle.h
        main_status_bar.h
        open_file_dialog.h
        show_message.h

        PRIVATE
        get_input_dialog.cpp
        main_menu_bar.cpp
        main_proc_handle.cpp
        main_status_bar.cpp
        open_file_dialog.cpp
        show_message.cpp
        )
add_subdirectory(base)
