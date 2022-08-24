add_subdirectory(assets_filter_widgets)
add_subdirectory(edit_widgets)
add_subdirectory(time_sequencer_widgets)

target_sources(doodle_lib

        PUBLIC
        time_sequencer_widget.h
        edit_widget.h
        subtitle_processing.h
        project_edit.h
        ue4_widget.h
        screenshot_widget.h
        opencv_player_widget.h
        assets_filter_widget.h
        csv_export_widgets.h
        assets_file_widgets.h
        extract_subtitles_widgets.h
        long_time_tasks_widget.h
        make_link_widget.h
        all_user_view_widget.h

        PRIVATE
        edit_widget.cpp
        assets_file_widgets.cpp
        csv_export_widgets.cpp
        assets_filter_widget.cpp
        extract_subtitles_widgets.cpp
        opencv_player_widget.cpp
        make_link_widget.cpp
        subtitle_processing.cpp
        long_time_tasks_widget.cpp
        project_edit.cpp
        time_sequencer_widget.cpp
        screenshot_widget.cpp
        ue4_widget.cpp
        all_user_view_widget.cpp
        )

