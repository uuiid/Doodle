target_sources(
        doodle_lib
        PUBLIC
        face_capture_types.h
        media_fragment_pipeline.h
        frame_filter.h
        capture_session.h
        frame_align.h

        PRIVATE
        media_fragment_pipeline.cpp
        frame_filter.cpp
        capture_session.cpp
        frame_align.cpp
)
